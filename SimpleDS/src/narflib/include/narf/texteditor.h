#ifndef NARF_TEXTEDITOR_H
#define NARF_TEXTEDITOR_H

#include <string>
#include <chrono>

namespace narf {
	class TextEditor {
	public:
		uint32_t cursor;
		size_t sel_length;
		TextEditor();
		~TextEditor();

		const std::string &getString() const { return str_; }

		void clear();
		void addString(const std::string &s);
		void setString(const std::string &s);
		void moveCursorLeft(int count = 1);
		void moveCursorRight(int count = 1);
		void moveCursor(int count);
		void delAtCursor(int count);
		void homeCursor();
		void endCursor();

		virtual void updated();
		std::chrono::time_point<std::chrono::system_clock> last_edited;

	private:
		std::string str_;
	};
} // namespace narf

#endif // NARF_TEXTEDITOR_H
