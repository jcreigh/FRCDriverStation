#include "narf/version.h"
#include <stdio.h>
#include <gtest/gtest.h>

int main(int argc, char **argv)
{
	printf("NarfLib unit tests\n");
	printf("Version: %d.%d%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_RELEASE);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

