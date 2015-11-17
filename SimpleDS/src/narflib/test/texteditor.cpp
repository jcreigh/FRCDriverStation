#include "narf/texteditor.h"
#include <gtest/gtest.h>

using namespace narf;

TEST(TextEditorTest, Simple) {
	TextEditor ed;

	EXPECT_EQ(0, ed.getString().length());

	ed.addString("hello");
	EXPECT_EQ("hello", ed.getString());
	EXPECT_EQ(5, ed.cursor);

	ed.moveCursor(-1);
	EXPECT_EQ(4, ed.cursor);

	ed.moveCursor(-1);
	EXPECT_EQ(3, ed.cursor);

	ed.moveCursor(-1);
	EXPECT_EQ(2, ed.cursor);

	ed.moveCursor(1);
	EXPECT_EQ(3, ed.cursor);

	ed.moveCursor(-1);
	EXPECT_EQ(2, ed.cursor);

	ed.moveCursor(-1);
	EXPECT_EQ(1, ed.cursor);

	ed.moveCursor(-1);
	EXPECT_EQ(0, ed.cursor);

	// add string at end
	ed.endCursor();
	ed.addString(" world");
	EXPECT_EQ("hello world", ed.getString());

	// move backwards
	ed.moveCursor(-6);
	EXPECT_EQ(5, ed.cursor);

	// add string in middle
	ed.addString(" cruel");
	EXPECT_EQ("hello cruel world", ed.getString());

	// home
	ed.homeCursor();
	EXPECT_EQ(0, ed.cursor);

	// delete forwards
	ed.delAtCursor(5);
	EXPECT_EQ(" cruel world", ed.getString());
	EXPECT_EQ(0, ed.cursor);

	// add string at front
	ed.addString("goodbye");
	EXPECT_EQ("goodbye cruel world", ed.getString());
	EXPECT_EQ(7, ed.cursor);

	// add string in middle after insert
	ed.addString("!");
	EXPECT_EQ("goodbye! cruel world", ed.getString());
	EXPECT_EQ(8, ed.cursor);

	// delete backwards
	ed.delAtCursor(-1);
	EXPECT_EQ("goodbye cruel world", ed.getString());
	EXPECT_EQ(7, ed.cursor);
}


TEST(TextEditorTest, Unicode) {
	TextEditor ed;

	ed.addString("\xC2\xA9");
	EXPECT_EQ(2, ed.cursor);

	ed.moveCursor(-1);
	EXPECT_EQ(0, ed.cursor);

	ed.addString("abc");
	EXPECT_EQ(3, ed.cursor);
	EXPECT_EQ("abc\xC2\xA9", ed.getString());

	// move cursor right across copyright symbol
	ed.moveCursor(1);
	EXPECT_EQ(5, ed.cursor);

	// move cursor left across copyright symbol
	ed.moveCursor(-1);
	EXPECT_EQ(3, ed.cursor);

	// insert text before multi-byte codepoint
	ed.addString("xyz");
	EXPECT_EQ("abcxyz\xC2\xA9", ed.getString());

	// insert text after multi-byte codepoint
	ed.endCursor();
	ed.addString("zyx");
	EXPECT_EQ("abcxyz\xC2\xA9zyx", ed.getString());
	EXPECT_EQ(11, ed.cursor);

	// move cursor left across single-byte codepoints
	ed.moveCursor(-3);
	EXPECT_EQ(8, ed.cursor);

	// move cursor left across multi-byte codepoint
	ed.moveCursor(-1);
	EXPECT_EQ(6, ed.cursor);

	// insert a 3-byte codepoint
	ed.addString("\xE2\x88\x9A");
	EXPECT_EQ("abcxyz\xE2\x88\x9A\xC2\xA9zyx", ed.getString());
	EXPECT_EQ(9, ed.cursor);

	// delete 3-byte codepoint before cursor
	ed.delAtCursor(-1);
	EXPECT_EQ("abcxyz\xC2\xA9zyx", ed.getString());
	EXPECT_EQ(6, ed.cursor);

	// delete 2-byte codepoint after cursor
	ed.delAtCursor(1);
	EXPECT_EQ("abcxyzzyx", ed.getString());
	EXPECT_EQ(6, ed.cursor);
}
