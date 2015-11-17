#ifndef NARF_UTF_H
#define NARF_UTF_H

#include <string>

namespace narf {

#ifdef _WIN32
	void toUTF8(const wchar_t* utf16, std::string& utf8);
	void toUTF8(const std::wstring& utf16, std::string& utf8);
	void toUTF16(const char* utf8, std::wstring& utf16);
	void toUTF16(const std::string& utf8, std::wstring& utf16);

	void appendUTF8(std::string& s, uint32_t c);
	void appendUTF16(std::wstring& s, uint32_t c);

	class UTF16Iter {
	private:
		const uint16_t* s_;
		size_t i_;
		uint32_t cur_;
		unsigned curLen_;

		uint16_t get16(unsigned offset) const;
		void decode();

	public:
		UTF16Iter(const uint16_t* s);

		bool operator!=(const UTF16Iter& other) const {
			return s_ + i_ != other.s_ + other.i_;
		}

		uint32_t operator*() const {
			return cur_;
		}

		const UTF16Iter& operator++();
	};

	class UTF16Iterator {
	private:
		const uint16_t* start_;
		const uint16_t* end_;

	public:
		// UTF16Iterator is only compiled on Win32, so wchar_t is always 16 bits
		UTF16Iterator(const wchar_t* utf16);
		UTF16Iterator(const std::wstring& utf16);

		UTF16Iter begin() const {
			return UTF16Iter(start_);
		}

		UTF16Iter end() const {
			return UTF16Iter(end_);
		}
	};

#endif // _WIN32

	class UTF8Iter {
	private:
		const uint8_t* s_;
		size_t i_;
		uint32_t cur_; // current codepoint
		unsigned curLen_; // length of current codepoint in bytes

		void decode();

	public:
		UTF8Iter(const uint8_t* s);

		bool operator!=(const UTF8Iter& other) const {
			return s_ + i_ != other.s_ + other.i_;
		}

		uint32_t operator*() const {
			return cur_;
		}

		const UTF8Iter& operator++();
	};

	class UTF8Iterator {
	private:
		const uint8_t* start_;
		const uint8_t* end_;
	public:
		UTF8Iterator(const char* utf8);
		UTF8Iterator(const std::string& utf8);

		UTF8Iter begin() const {
			return UTF8Iter(start_);
		}

		UTF8Iter end() const {
			return UTF8Iter(end_);
		}
	};

	static inline bool isUTF8StartByte(uint8_t b) {
		// valid start bytes are binary 0xxxxxxx or 11xxxxxx
		return ((b & 0x80u) == 0) || ((b & 0xC0u) == 0xC0u);
	}

	// returns size in bytes of first codepoint in utf8
	int UTF8CharSize(const char* utf8);
	int UTF8CharSize(uint32_t codepoint);

	// returns size in bytes of codepoint before offset in utf8
	int UTF8PrevCharSize(const char* utf8, size_t offset);
}

#endif // NARF_UTF_H
