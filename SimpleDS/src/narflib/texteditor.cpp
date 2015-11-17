#include "narf/texteditor.h"
#include "narf/console.h"
#include "narf/utf.h"
#include <chrono>
#include <cstdlib>
#include <assert.h>

narf::TextEditor::TextEditor() {
	homeCursor();
}

narf::TextEditor::~TextEditor() {
}

void narf::TextEditor::updated() {
	last_edited = std::chrono::system_clock::now();
}

void narf::TextEditor::clear() {
	str_.clear();
	cursor = 0;
	updated();
}

void narf::TextEditor::addString(const std::string &s) {
	str_.insert(cursor, s);
	cursor += s.length();
	updated();
}

void narf::TextEditor::setString(const std::string &s) {
	str_ = s;
	cursor = static_cast<uint32_t>(str_.size());
	updated();
}

void narf::TextEditor::moveCursorLeft(int count) {
	if (cursor == 0) {
		return;
	}

	while (count-- > 0) {
		assert(cursor > 0);
		auto size = narf::UTF8PrevCharSize(str_.c_str(), cursor);
		if (size <= 0) {
			return;
		}

		cursor -= size;
	}
	updated();
}

void narf::TextEditor::moveCursorRight(int count) {
	if (cursor == str_.size()) {
		return;
	}
	while (count-- > 0) {
		auto size = narf::UTF8CharSize(&str_[cursor]);
		if (size <= 0) {
			return;
		}
		cursor += size;
	}
	assert(cursor <= str_.size());
	updated();
}

void narf::TextEditor::moveCursor(int count) {
	if (count < 0) {
		moveCursorLeft(-count);
	} else {
		moveCursorRight(count);
	}
}

void narf::TextEditor::delAtCursor(int count) {
	if (count < 0) {
		moveCursor(count);
	}
	count = abs(count);
	while (count-- > 0) {
		str_.erase(cursor, narf::UTF8CharSize(&str_[cursor]));
	}
	updated();
}

void narf::TextEditor::homeCursor() {
	cursor = 0;
	updated();
}

void narf::TextEditor::endCursor() {
	cursor = static_cast<uint32_t>(str_.size());
	updated();
}
