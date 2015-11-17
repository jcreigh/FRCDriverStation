#include "narf/console.h"
#include "narf/path.h"

#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <stdlib.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef _WIN32

#include <windows.h>
#include <shlobj.h>
#include "narf/utf.h"

const std::string narf::util::DirSeparator("\\");
#else
const std::string narf::util::DirSeparator("/");
#endif


#ifdef linux

static std::string readLink(const std::string &path) {
	size_t len = 1024;
	size_t lenStep = 1024;
	size_t maxLen = 1024 * 4096;

	do {
		char *buf = new char[len + 1];
		if (!buf) {
			return std::string("");
		}

		auto bytes = readlink(path.c_str(), buf, len + 1);
		if (bytes < 0) {
			return std::string("");
		}

		if (static_cast<size_t>(bytes) > len) {
			// try again with larger buffer
			len += lenStep;
			delete [] buf;
		} else {
			buf[bytes] = '\0';
			auto s = std::string(buf);
			delete [] buf;
			return s;
		}
	} while (len <= maxLen);

	return std::string("");
}


std::string narf::util::exeName() {
	return readLink("/proc/self/exe");
}

#endif // linux

#if defined(__unix__) && !defined(linux)
std::string narf::util::exeName() {
	return "";
}
#endif // unix && !linux


#if defined(__unix__) || defined(__APPLE__)

std::string narf::util::userConfigDir(const std::string& appName) {
	// look up home dir and append .config/<appName>
	char* home;
	home = getenv("HOME");
	// TODO: sanitize/validate $HOME?
	if (!home) {
		// TODO: fall back to getpwuid() (use getpwuid_r?)
	}

	if (home) {
		return std::string(home) + "/.config/" + appName;
	}
	return "";
}


static mode_t getPathStatMode(const std::string& path) {
	struct stat st;
	if (stat(path.c_str(), &st) == -1) {
		return 0;
	}
	return st.st_mode;
}


bool narf::util::dirExists(const std::string& path) {
	auto mode = getPathStatMode(path);
	return mode != 0 && (mode & S_IFMT) == S_IFDIR;
}


bool narf::util::fileExists(const std::string& path) {
	auto mode = getPathStatMode(path);
	return mode != 0 && (mode & S_IFMT) == S_IFREG;
}


void narf::util::rename(const std::string& path, const std::string& newPath) {
	::rename(path.c_str(), newPath.c_str());
}


bool narf::util::createDir(const std::string& path) {
	return ::mkdir(path.c_str(), 0755) == 0;
}

#endif // unix

#ifdef _WIN32

std::string narf::util::exeName() {
	const DWORD len = 32768; // max NT-style name length + terminator
	wchar_t buffer[len];

	DWORD ret = GetModuleFileNameW(NULL, buffer, len);
	if (ret == 0 || ret >= len) {
		return ""; // error
	}

	std::string result;
	narf::toUTF8(buffer, result);
	return result;
}


std::string narf::util::userConfigDir(const std::string& appName) {
	wchar_t buffer[MAX_PATH];
	HRESULT hr = SHGetFolderPathW(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, buffer);
	if (!SUCCEEDED(hr)) {
		return ""; // error
	}
	std::string result;
	narf::toUTF8(buffer, result);
	return result + "\\" + appName;
}


static DWORD getPathAttrs(const std::string& path) {
	std::wstring pathW;
	narf::toUTF16(path, pathW);
	return GetFileAttributesW(pathW.c_str());
}


bool narf::util::dirExists(const std::string& path) {
	auto attrs = getPathAttrs(path);
	return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}


bool narf::util::fileExists(const std::string& path) {
	auto attrs = getPathAttrs(path);
	return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}


void narf::util::rename(const std::string& path, const std::string& newPath) {
	std::wstring pathW, newPathW;
	narf::toUTF16(path, pathW);
	narf::toUTF16(newPath, newPathW);
	MoveFileW(pathW.c_str(), newPathW.c_str());
}


bool narf::util::createDir(const std::string& path) {
	std::wstring pathW;
	narf::toUTF16(path, pathW);
	return ::CreateDirectoryW(pathW.c_str(), nullptr) == 0;
}

#endif // _WIN32

#ifdef __APPLE__

std::string narf::util::exeName() {
	char buffer[32768];
	uint32_t len = sizeof(buffer);

	if (_NSGetExecutablePath(buffer, &len) == 0) {
		return std::string(buffer);
	}

	return ""; // error
}

#endif // __APPLE__


std::string narf::util::exeDir() {
	return dirName(exeName());
}


std::string narf::util::appendPath(const std::string& path, const std::string& append) {
	if (path.back() == DirSeparator[0]) {
		return path + append;
	} else {
		return path + DirSeparator + append;
	}
}


bool narf::util::createDirs(const std::string& path) {
	if (dirExists(path)) {
		return true; // We're already a directory
	}
	if (!createDirs(dirName(path))) {
		return false; // Failed to create one of the parent directories
	}
	return createDir(path);
}


static size_t pathLastSlash(const std::string& path, size_t* trimmedLen = nullptr, size_t* baseNameLen = nullptr) {
	auto len = path.length();
	while (len > 0 && path[len - 1] == narf::util::DirSeparator[0]) {
		len--;
	}
	if (trimmedLen) {
		*trimmedLen = len;
	}
	auto lastSlash = path.rfind(narf::util::DirSeparator, len - 1);
	if (baseNameLen && lastSlash != std::string::npos) {
		*baseNameLen = len - lastSlash - 1;
	}
	return lastSlash;
}


std::string narf::util::dirName(const std::string& path) {
	size_t trimmedLen;
	auto lastSlash = pathLastSlash(path, &trimmedLen);
	if (lastSlash != std::string::npos) {
		auto dir = path.substr(0, lastSlash == 0 ? 1 : lastSlash);
#ifdef _WIN32
		if (dir.length() == 2 && dir[1] == ':') {
			// "C:" -> "C:\"
			dir += DirSeparator;
		}
		if (dir[0] == '\\' && dir[1] == '\\') {
			// UNC path "\\server\share"
			int slashCount = 0;
			for (size_t i = 0; i <= dir.length(); i++) {
				if (dir[i] == '\\') {
					slashCount++;
					if (slashCount >= 3) {
						return dir;
					}
				}
			}

			// less than 3 backslashes found (missing share)
			// return original path with trailing slashes trimmed
			return path.substr(0, trimmedLen);
		}
#endif
		return dir;
	}
	return path;
}


std::string narf::util::baseName(const std::string& path) {
	size_t baseNameLen;
	auto lastSlash = pathLastSlash(path, nullptr, &baseNameLen);
	if (path.length() > 1 && lastSlash != std::string::npos) {
		return path.substr(lastSlash + 1, baseNameLen);
	}
	return path;
}
