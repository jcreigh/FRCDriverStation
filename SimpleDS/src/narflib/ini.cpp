/*
 * INI file load/save
 *
 * Copyright (c) 2015 Daniel Verkamp, Jessica Creighton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#include "narf/ini.h"
#include "narf/console.h"

#include <errno.h>
#include <cstdlib>

static bool isSpace(char c) {
	return c == ' ' || c == '\t';
}


static bool isNewLine(char c) {
	return c == '\r' || c == '\n';
}

void narf::INI::warn(const std::string& s) {
	// TODO: Figure out what to do with this
	printf("%s\n", s.c_str());
	//narf::console->println(s);
}

narf::INI::Line::Line(const char* data, size_t size) {
	std::string line(data, size);
	raw = line;
	parse();
}

narf::INI::Line::Line(const std::string& line) {
	raw = line;
	parse();
}

narf::INI::Line::~Line() { }

std::string narf::INI::Line::getKey() {
	return key;
}

std::string narf::INI::Line::getValue() {
	return value;
}

std::string narf::INI::Line::getRaw() {
	return raw;
}

narf::INI::Line::Type narf::INI::Line::getType() {
	return lineType;
}

std::string narf::INI::Line::setValue(std::string newValue) {
	// TODO: Make this overwrite any whitespace before an inline comment, to maintain its position
	if (value == newValue) {
		return value;
	}
	std::string oldValue = value;
	value = "";
	for (size_t i = 0; i < newValue.size(); i++) {
		char c = newValue.at(i);
		std::unordered_map<char, std::string> escapes {{'\0', "\\0"}, {'\a', "\\a"}, {'\b', "\\b"},
				{'\t', "\\t"}, {'\r', "\\r"}, {'\n', "\\n"}, {';', "\\;"}, {'"', "\\\""}, {'\\', "\\\\"}};
		if (escapes.count(c) > 0) {
			value += escapes.at(c);
		} else if (c < 32 || (uint8_t)c >= 127) {
			char raw[30];
			snprintf(raw, sizeof(raw), "\\x%02x", (uint8_t)c); // TODO: Unicode? Or at least latin-1?
			value += raw;
		} else if (c == ' ' && (i + 1 == newValue.size())) { // Last character is a space, so we need quotes
			value += " \""; // The last space, plus the quote
			value.insert(value.begin(), '"'); // First space
		} else {
			value += c;
		}
	}
	raw = raw.substr(0, valueStartPos) + value + raw.substr(valueStartPos + valueLength);
	valueLength = value.size();
	return oldValue;
}


void narf::INI::Line::parse() {
	enum class State {
		BeginLine,
		EndLine,
		Ignore,
		Section,
		Key,
		Equals,
		Value,
	};
	State state = State::BeginLine;
	enum class QuoteState { None, Inside, Done };
	QuoteState quoteState = QuoteState::None;
	enum class EscapeState { None, Hex, Other }; // To assist with multicharacter escapes
	EscapeState escapeState = EscapeState::None;
	const char* keyStart = nullptr;
	const char* valueStart = nullptr;
	const char* sectionStart = nullptr;
	const char* chars = raw.c_str();
	size_t size = raw.size();
	error = false; // No parse errors
	const char* valueEnd = nullptr;
	std::string hexEscape;
	size_t i = 0;
	char c;
	for (; i <= size; i++) {
		const char* d;
		if (i == size) {
			c = '\0';
			d = &chars[size - 1];
		} else {
			c = chars[i];
			d = &chars[i];
		}
		switch (state) {
			case State::BeginLine:
				if (c == '[') {
					state = State::Section;
					lineType = Type::Section;
				} else if (isSpace(c)) {
					// eat whitespace
				} else if (isNewLine(c)) {
					// Blank line, no need to go further
					lineType = Type::Other;
				} else if (c == ';') {
					state = State::Ignore;
					lineType = Type::Comment;
				} else {
					state = State::Key;
					keyStart = d;
					lineType = Type::Entry;
				};
				break;
			case State::EndLine:
				if (c == '\0' || isNewLine(c)) {
					// We done
				} else if (isSpace(c)) {
					// skip whitespace
				} else if (c == ';') {
					// Rest of line is a comment
					state = State::Ignore;
				} else {
					warn(("found junk character '" + (std::string() + c) +  "' at end of line").c_str());
					error = true;
					state = State::Ignore;
				}
				break;
			case State::Section:
				if (sectionStart == nullptr) {
					sectionStart = d;
				}
				if (c == ']') {
					key = std::string(sectionStart, static_cast<size_t>(d - sectionStart));
					state = State::EndLine;
				}
				break;
			case State::Key:
				if (isSpace(c) || c == '=') {
					if (c == '=') {
						state = State::Value;
					} else {
						state = State::Equals;
					}
					key = std::string(keyStart, static_cast<size_t>(d - keyStart));
					keyStart = nullptr;
				} else if (c == '\0' || isNewLine(c)) {
					warn(("key \"" + key + "\" without value").c_str());
					error = true;
					state = State::Ignore;
				} else {
					// Accumulate characters into key
					// TODO: Are there invalid key characters?
				}
				break;
			case State::Equals:
				if (isSpace(c)) {
					// Eat whitespace
				} else if (c == '=') {
					state = State::Value;
				} else {
					error = true;
					warn("junk after key");
					state = State::Ignore;
				}
				break;
			case State::Value:
				if (valueStart == nullptr) {
					if (isSpace(c)) {
						// eat whitespace between = and value
					} else {
						valueEnd = nullptr;
						valueStart = d;
						valueStartPos = i;
						valueLength = 0;
						i--;
					}
				} else if (c == '\0' || isNewLine(c) || quoteState == QuoteState::Done || (c == ';' && quoteState != QuoteState::Inside && escapeState == EscapeState::None)) {
					if (escapeState != EscapeState::None) {
						if (escapeState == EscapeState::Hex) { // We were at the start
							if (hexEscape.size() > 0) {
								value += (char)std::stoi(hexEscape, nullptr, 16); // Reached the end in the middle of a hex escape
							} else { // Should this error?
								value += "\\x"; // We started a hex escape, but reached the end before we even started
							}
						} else {
							value += '\\'; // We started escaping but got to the end, so treat it as a normal slash
						}
					}
					// Trim trailing whitespace. std::string doesn't have a built in trim for some reason :|
					// From here: http://stackoverflow.com/a/17976541
					if (quoteState == QuoteState::None) {
						// We weren't inside of a quote, so we should trim trailing spaces
						auto trimmedback = std::find_if_not(value.rbegin(), value.rend(), [](char c){return isSpace(c);}).base();
						value = std::string(value.begin(), trimmedback);
					}
					valueLength = (size_t)(valueEnd + 1 - valueStart);
					state = State::EndLine;
					i--;
					continue;
				} else {
					// Accumulate any other chars into value
					if (!isSpace(c)) {
						valueEnd = d; // Store the last non-space character so our valueLength is accurate after trimming
					}
					if (c == '\\' && escapeState == EscapeState::None) {
						escapeState = EscapeState::Other;
					} else if (escapeState == EscapeState::Other) {
						std::unordered_map<char, char> escapes {{'0', '\0'}, {'a', '\a'}, {'b', '\b'}, {'t', '\t'}, {'r', '\r'}, {'n', '\n'}};
						if (escapes.count(c) > 0) {
							value += escapes.at(c);
						} else if (c == 'x') { // Hex escape
							escapeState = EscapeState::Hex;
							hexEscape = "";
						} else { // Catches Quote, Semicolon, and Backslash
							value += c;
						}
						if (escapeState == EscapeState::Other) {
							escapeState = EscapeState::None;
						}
					} else if (escapeState == EscapeState::Hex) {
						if (hexEscape.size() < 2 && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
							hexEscape += c;
						} else {
							if (hexEscape.size() > 0) {
								value += (char)std::stoi(hexEscape, nullptr, 16);
							} else { // This should probably error?
								value += "\\x";
							}
							escapeState = EscapeState::None;
							i--; // Redo the previous character if not part of the hex
							continue;
						}
					} else if (c == '"') {
						switch (quoteState) {
							case QuoteState::None:
								if (valueStart == d) {
									quoteState = QuoteState::Inside;
								} else {
									error = true;
									value += '"';
								}
								break;
							case QuoteState::Inside:
								quoteState = QuoteState::Done;
								break;
							case QuoteState::Done:
								error = true;
								break;
						}
					} else if (quoteState != QuoteState::Done){
						value += c;
					}
				}
				break;
			case State::Ignore:
				// Nothing matters </3
				break;
		}
		if (c == '\0' || isNewLine(c)) {
			break;
		}
	}
	raw.resize(i + (c != '\0' ? 1 : 0)); // Truncate to just the bits we've read, ignoring last \0
}

bool narf::INI::Line::hasError() {
	return error;
}

narf::INI::File::File() { }

narf::INI::File::~File() { }

bool narf::INI::File::load(std::string data) {
	return load(data.c_str(), data.size());
}

bool narf::INI::File::load(const void* data, size_t size) {
	const char* chars = static_cast<const char*>(data);
	size_t i = 0;
	std::string section;
	bool error = false;
	while (i < size) {
		narf::INI::Line line(chars + i, size - i);
		error |= line.hasError();
		lines.push_back(line);
		i += line.getRaw().size();
		if (line.getType() == narf::INI::Line::Type::Section) {
			section = line.getKey() + ".";
		} else if (line.getType() == narf::INI::Line::Type::Entry) {
			setString(section + line.getKey(), line.getValue());
		} else {
			// Comment or blank line or something
		}
	}
	return !error;
}

std::string narf::INI::File::save() {
	// It ain't pretty

	std::string section;
	std::string output;
	std::vector<std::string> savedKeys;
	// First update lines with new values and keep track of which keys exist
	for (auto &line : lines) {
		if (line.getType() == narf::INI::Line::Type::Section) {
			section = line.getKey() + ".";
		} else if (line.getType() == narf::INI::Line::Type::Entry) {
			line.setValue(getString(section + line.getKey()));
			savedKeys.push_back(section + line.getKey());
		} else {
			// Comment or blank line or something
		}
	}
	// Iterate over all values and add ones which don't exist as lines
	for (auto &pair : values_) {
		auto key = pair.first;
		if (std::count(savedKeys.begin(), savedKeys.end(), key) == 0) {
			std::string section;
			size_t lastEntry = 0;
			for (size_t i = 0; i < lines.size(); i++) {
				auto line = lines.at(i);
				if (line.getType() == narf::INI::Line::Type::Section) {
					if ((section == "" && key.find('.') == std::string::npos) ||
							(section != "" && key.find(section + ".") == 0)) {
						// If we were at global and key is at global
						// or if we got to the end of the section which matches our key
						// then insert and break
						break;
					}
					section = line.getKey();
				}
				if (line.getType() != narf::INI::Line::Type::Other) {
					lastEntry = (size_t)(i + 1);
				}
			}
			if ((section != "" && key.find(section + ".") == std::string::npos) || (section == "" && key.find(".") != std::string::npos)) {
				// TODO: Detect which line endings to use
				section = key.substr(0, key.find("."));
				lines.insert(lines.begin() + (int)lastEntry, narf::INI::Line("[" + section + "]\n"));
				lastEntry++;
			}
			auto dotPos = key.find(".");
			if (dotPos == std::string::npos) {
				dotPos = 0;
			} else {
				dotPos++;
			}
			// TODO: Detect indentation?
			narf::INI::Line newLine((section == "" ? "" : "\t") + key.substr(dotPos, std::string::npos) + " = foo\n");
			newLine.setValue(pair.second);
			lines.insert(lines.begin() + (int)lastEntry, newLine);
		}
	}
	// Iterate again for the output
	for (auto &line : lines) {
		output += line.getRaw();
	}
	return output;
}

bool narf::INI::File::remove(const std::string& key) {
	// TODO: Remove section if removing key causes it to be empty?
	// TODO: Add a removeSection() in order to remove an entirie section?
	if (!has(key)) {
		return false;
	}
	values_.erase(key);
	std::string section;
	for (size_t i = 0; i < lines.size(); i++) {
		auto line = lines[i];
		switch (line.getType()) {
			case narf::INI::Line::Type::Section:
				section = line.getKey() + ".";
				break;
			case narf::INI::Line::Type::Entry:
				if (key == section + line.getKey()) {
					lines.erase(lines.begin() + (int)i);
					i--;
				}
				break;
			default:
				break;
		}
	}
	return true;
}

std::string narf::INI::File::getString(const std::string& key) const {
	if (has(key)) {
		return values_.at(key);
	}
	// TODO: throw exception if key not found?
	return "";
}


void narf::INI::File::setString(const std::string& key, const std::string& value) {
	values_[key] = value;
	update(key);
}


void narf::INI::File::update(const std::string& key) {
	updateSignal.emit(key);
	//if (updateHandler) {
		//updateHandler(key);
	//}
}


bool narf::INI::File::has(const std::string& key) const {
	return values_.count(key) != 0;
}


std::vector<std::string> narf::INI::File::getKeys() const {
	std::vector<std::string> keys;
	for (auto& iter : values_) {
		keys.push_back(iter.first);
	}
	std::sort(keys.begin(), keys.end());
	return keys;
}


bool narf::INI::File::getBool(const std::string& key) const {
	auto raw = getString(key);
	switch (raw[0]) {
	case 'y': // yes
	case 'Y':
	case 't': // true
	case 'T':
	case 'e': // enabled
	case 'E':
	case '1':
		return true;
	case 'o':
	case 'O':
		switch (raw[1]) {
		case 'n': // on
		case 'N':
			return true;
		}
		break;
	}
	return false;
}


double narf::INI::File::getDouble(const std::string& key) const {
	auto raw = getString(key);
	char* end;
	// TODO: deal with locale stuff - should this use a locale-independent format?
	errno = 0;
	auto value = strtod(raw.c_str(), &end);
	if (errno == 0 && *end == '\0') {
		return value;
	}
	// TODO: throw exception?
	return 0.0;
}


float narf::INI::File::getFloat(const std::string& key) const {
	return (float)getDouble(key);
}


int32_t narf::INI::File::getInt32(const std::string& key) const {
	auto raw = getString(key);
	char* end;
	errno = 0;
	auto value = strtol(raw.c_str(), &end, 0);
	if (errno == 0 && *end == '\0' && value <= INT32_MAX && value >= INT32_MIN) {
		return (int32_t)value;
	}
	warn(("bad int32 '" + raw + "'").c_str());
	// TODO: exception?
	return 0;
}


uint32_t narf::INI::File::getUInt32(const std::string& key) const {
	auto raw = getString(key);
	char* end;
	errno = 0;
	auto value = strtoul(raw.c_str(), &end, 0);
	if (errno == 0 && *end == '\0' && value <= UINT32_MAX) {
		return (uint32_t)value;
	}
	warn(("bad uint32 '" + raw + "'").c_str());
	// TODO: exception?
	return 0;
}


void narf::INI::File::setBool(const std::string& key, bool value) {
	setString(key, value ? "true" : "false");
}


void narf::INI::File::setDouble(const std::string& key, double value) {
	char raw[30];
	snprintf(raw, sizeof(raw), "%.17g", value);
	setString(key, raw);
}


void narf::INI::File::setFloat(const std::string& key, float value) {
	char raw[20];
	snprintf(raw, sizeof(raw), "%.9g", value);
	setString(key, raw);
}


void narf::INI::File::setInt32(const std::string& key, int32_t value) {
	char raw[20];
	snprintf(raw, sizeof(raw), "%" PRId32, value);
	setString(key, raw);
}


void narf::INI::File::initBool(const std::string& key, bool defaultValue) {
	if (has(key)) {
		update(key);
	} else {
		setBool(key, defaultValue);
	}
}


void narf::INI::File::initString(const std::string& key, const std::string& defaultValue) {
	if (has(key)) {
		update(key);
	} else {
		setString(key, defaultValue);
	}
}


void narf::INI::File::initDouble(const std::string& key, double defaultValue) {
	if (has(key)) {
		update(key);
	} else {
		setDouble(key, defaultValue);
	}
}


void narf::INI::File::initFloat(const std::string& key, float defaultValue) {
	if (has(key)) {
		update(key);
	} else {
		setFloat(key, defaultValue);
	}
}


void narf::INI::File::initInt32(const std::string& key, int32_t defaultValue) {
	if (has(key)) {
		update(key);
	} else {
		setInt32(key, defaultValue);
	}
}


std::string narf::INI::File::getString(const std::string& key, const std::string& defaultValue) const {
	return has(key) ? getString(key) : defaultValue;
}


bool narf::INI::File::getBool(const std::string& key, bool defaultValue) const {
	return has(key) ? getBool(key) : defaultValue;
}


double narf::INI::File::getDouble(const std::string& key, double defaultValue) const {
	return has(key) ? getDouble(key) : defaultValue;
}


float narf::INI::File::getFloat(const std::string& key, float defaultValue) const {
	return has(key) ? getFloat(key) : defaultValue;
}


int32_t narf::INI::File::getInt32(const std::string& key, int32_t defaultValue) const {
	return has(key) ? getInt32(key) : defaultValue;
}

uint32_t narf::INI::File::getUInt32(const std::string& key, uint32_t defaultValue) const {
	return has(key) ? getUInt32(key) : defaultValue;
}

