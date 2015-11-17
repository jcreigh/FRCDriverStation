#include <gtest/gtest.h>
#include "narf/embed.h"
DECLARE_EMBED(bar_txt)

TEST(EMBED, bar) {
	std::string bar = EMBED_STRING(bar_txt);
	ASSERT_EQ("Foo\n", bar);
}
