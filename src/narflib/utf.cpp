#include "narf/utf.h"

#include <cstring>

#ifdef _WIN32
void narf::toUTF8(const wchar_t* utf16, std::string& utf8) {
	utf8.clear();
	narf::UTF16Iterator i(utf16);
	for (const auto& c : i) {
		appendUTF8(utf8, c);
	}
}


void narf::toUTF8(const std::wstring& utf16, std::string& utf8) {
	toUTF8(utf16.c_str(), utf8);
}


void narf::toUTF16(const char* utf8, std::wstring& utf16) {
	utf16.clear();
	narf::UTF8Iterator i(utf8);
	for (const auto& c : i) {
		appendUTF16(utf16, c);
	}
}


void narf::toUTF16(const std::string& utf8, std::wstring& utf16) {
	toUTF16(utf8.c_str(), utf16);
}


void narf::appendUTF8(std::string& s, uint32_t c) {
	if (c < 0x80u) {
		s += (char)c;
		return;
	}

	if (c < 0x800u) {
		s += (char)((c >> 6) | 0xC0u);
		s += (char)((c & 0x3Fu) | 0x80u);
		return;
	}

	if (c < 0x10000u) {
		s += (char)((c >> 12) | 0xE0u);
		s += (char)(((c >> 6) & 0x3Fu) | 0x80u);
		s += (char)((c & 0x3Fu) | 0x80u);
		return;
	}

	if (c < 0x200000u) {
		s += (char)((c >> 18) | 0xF0u);
		s += (char)(((c >> 12) & 0x3Fu) | 0x80u);
		s += (char)(((c >> 6) & 0x3Fu) | 0x80u);
		s += (char)((c & 0x3Fu) | 0x80u);
		return;
	}

	s += "\xEF\xBF\xBD";
}


void narf::appendUTF16(std::wstring& s, uint32_t c) {
	if ((c >= 0xD800u && c <= 0xDFFFu) || (c > 0x10FFFFu)) {
		s += L'\xFFFD';
	} else if (c <= 0xFFFFu) {
		s += (wchar_t)(c);
	} else {
		c -= 0x10000;
		s += (wchar_t)((c >> 10) + 0xD800u);
		s += (wchar_t)((c & 0x3FFu) + 0xDC00u);
	}
}


narf::UTF16Iter::UTF16Iter(const uint16_t* s) : s_(s), i_(0) {
	decode();
}


void narf::UTF16Iter::decode() {
	uint16_t w1, w2;

	curLen_ = 1;
	w1 = s_[i_];
	if (w1 < 0xD800u || w1 > 0xDFFFu) {
		cur_ = w1;
		return;
	}

	curLen_ = 2;
	w2 = s_[i_ + 1];
	if (w1 > 0xDBFFu || w2 < 0xDC00u || w2 > 0xDFFFu) {
		// error!
		cur_ = 0xFFFD;
	} else {
		cur_ = (((w1 & 0x3FFu) << 10) | (w2 & 0x3FFu)) + 0x10000u;
	}
}


const narf::UTF16Iter& narf::UTF16Iter::operator++() {
	i_ += curLen_;
	decode();
	return *this;
}


narf::UTF16Iterator::UTF16Iterator(const wchar_t* utf16) :
	start_(reinterpret_cast<const uint16_t*>(utf16)),
	end_(reinterpret_cast<const uint16_t*>(utf16 + wcslen(utf16))) {
}


narf::UTF16Iterator::UTF16Iterator(const std::wstring& utf16) : UTF16Iterator(utf16.c_str()) {
}

#endif // _WIN32


narf::UTF8Iter::UTF8Iter(const uint8_t* utf8) : s_(utf8), i_(0) {
	decode();
}



void narf::UTF8Iter::decode() {
	uint32_t c0, c1, c2, c3;

	curLen_ = 1;
	c0 = s_[i_];
	if (c0 < 0x80u) {
		cur_ = c0;
		return;
	}

	if ((c0 & 0xC0u) != 0xC0u) {
		goto invalid;
	}

	curLen_ = 2;
	c1 = s_[i_ + 1];
	if ((c1 & 0xC0u) != 0x80u) {
		goto invalid;
	}

	if (c0 < 0xE0u) {
		// 2-byte sequence
		cur_ = ((c0 & 0x1Fu) << 6) | (c1 & 0x3Fu);
		return;
	}

	curLen_ = 3;
	c2 = s_[i_ + 2];
	if ((c2 & 0xC0u) != 0x80u) {
		goto invalid;
	}

	if (c0 < 0xF0u) {
		// 3-byte sequence
		cur_ = ((c0 & 0xFu) << 12) | ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu);
		return;
	}

	curLen_ = 4;
	c3 = s_[i_ + 3];
	if ((c3 & 0xC0u) != 0x80u) {
		goto invalid;
	}

	if (c0 < 0xF8u) {
		// 4-byte sequence
		cur_ = ((c0 & 0x7u) << 18) | ((c1 & 0x3Fu) << 12) | ((c2 & 0x3Fu) << 6) | (c3 & 0x3Fu);
		return;
	}

	// fall through for 5-byte/6-byte sequence (invalid)
invalid:
	// resync to next 0xxxxxxx or 11xxxxxx
	auto c = s_[i_ + curLen_ - 1];
	while (!isUTF8StartByte(c)) {
		curLen_++;
		c = s_[i_ + curLen_ - 1];
	}
	cur_ = 0xFFFDu;
}


const narf::UTF8Iter& narf::UTF8Iter::operator++() {
	i_ += curLen_;
	decode();
	return *this;
}


narf::UTF8Iterator::UTF8Iterator(const char* utf8) :
	start_(reinterpret_cast<const uint8_t*>(utf8)),
	end_(reinterpret_cast<const uint8_t*>(utf8 + strlen(utf8))) {
}


narf::UTF8Iterator::UTF8Iterator(const std::string& utf8) : UTF8Iterator(utf8.c_str()) {
}


int narf::UTF8CharSize(const char* utf8) {
	unsigned c0 = *(const uint8_t*)utf8;
	if (c0 == 0) {
		return 0;
	} else if (c0 < 0x80u) {
		return 1;
	} else if (c0 < 0xE0u) {
		return 2;
	} else if (c0 < 0xF0u) {
		return 3;
	} else if (c0 < 0xF8u) {
		return 4;
	} else {
		return -1; // invalid encoding
	}
}


int narf::UTF8CharSize(uint32_t codepoint) {
	if (codepoint < 0x80u) {
		return 1;
	} else if (codepoint < 0x800u) {
		return 2;
	} else if (codepoint < 0x1000u) {
		return 3;
	} else if (codepoint < 0x200000u) {
		return 4;
	} else {
		return -1; // invalid encoding
	}
}


int narf::UTF8PrevCharSize(const char* utf8, size_t offset) {
	auto s = (const uint8_t*)utf8;

	if (offset == 0) {
		return 0;
	}

	if (!isUTF8StartByte(s[offset])) {
		return -1;
	}

	size_t origOffset = offset;

	// resync to previous 0xxxxxxx or 11xxxxxx
	while (!isUTF8StartByte(s[--offset])) {
		if (offset == 0) {
			break;
		}
	}
	return (int)(origOffset - offset);
}
