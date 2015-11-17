#include "narf/utf.h"
#include <gtest/gtest.h>

using namespace narf;

TEST(UTF8CharSizeTest, Valid) {
	EXPECT_EQ(0, UTF8CharSize(""));
	EXPECT_EQ(1, UTF8CharSize("aZZZ"));
	EXPECT_EQ(2, UTF8CharSize("\xC2\xA9ZZZ")); // copyright symbol
	EXPECT_EQ(3, UTF8CharSize("\xE2\x88\x9AZZZ")); // square root
	EXPECT_EQ(4, UTF8CharSize("\xF0\xA0\x9C\x8EZZZ"));
}


TEST(UTF8PrevCharSize, Valid) {
	EXPECT_EQ(0, UTF8PrevCharSize("", 0));
	EXPECT_EQ(0, UTF8PrevCharSize("aaa", 0));
	EXPECT_EQ(1, UTF8PrevCharSize("aaa", 1));
	EXPECT_EQ(1, UTF8PrevCharSize("aaa", 2));
	EXPECT_EQ(0, UTF8PrevCharSize("Z\xC2\xA9ZZZ", 0));
	EXPECT_EQ(1, UTF8PrevCharSize("Z\xC2\xA9ZZZ", 1));
	EXPECT_EQ(-1, UTF8PrevCharSize("Z\xC2\xA9ZZZ", 2));
	EXPECT_EQ(2, UTF8PrevCharSize("Z\xC2\xA9ZZZ", 3));
	EXPECT_EQ(2, UTF8PrevCharSize("\xC2\xA9ZZZ", 2));
	EXPECT_EQ(1, UTF8PrevCharSize("\xC2\xA9ZZZ", 3));
	EXPECT_EQ(3, UTF8PrevCharSize("\xE2\x88\x9AZZZ", 3));
	EXPECT_EQ(4, UTF8PrevCharSize("\xF0\xA0\x9C\x8EZZZ", 4));
	EXPECT_EQ(2, UTF8PrevCharSize("abc\xC2\xA9", 5));
}
