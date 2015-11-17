#include <gtest/gtest.h>
#include "narf/format.h"

TEST(Format, format) {
	std::string s = narf::util::format("%s %d %.2f %x", "foo", -12, 12.341, 0xcf);
	ASSERT_EQ("foo -12 12.34 cf", s);
}
