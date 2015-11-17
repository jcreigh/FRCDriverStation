#include <gtest/gtest.h>
#include "narf/path.h"

#if defined(__unix__) || defined(__APPLE__)

TEST(PATH, dirName) {
	ASSERT_EQ("/", narf::util::dirName("/"));
	ASSERT_EQ("/", narf::util::dirName("/foo"));
	ASSERT_EQ("/", narf::util::dirName("/foo/"));
	ASSERT_EQ("/foo", narf::util::dirName("/foo/bar"));
}

TEST(PATH, baseName) {
	ASSERT_EQ("/", narf::util::baseName("/"));
	ASSERT_EQ("foo", narf::util::baseName("foo"));
	ASSERT_EQ("foo", narf::util::baseName("/foo/"));
	ASSERT_EQ("bar", narf::util::baseName("/foo/bar"));
}

#elif defined(_WIN32)

TEST(PATH, dirName) {
	ASSERT_EQ("C:\\", narf::util::dirName("C:\\"));
	ASSERT_EQ("C:\\", narf::util::dirName("C:\\foo"));
	ASSERT_EQ("C:\\", narf::util::dirName("C:\\foo\\"));
	ASSERT_EQ("C:\\foo", narf::util::dirName("C:\\foo\\bar"));
	ASSERT_EQ("\\\\server\\share", narf::util::dirName("\\\\server\\share"));
	ASSERT_EQ("\\\\server\\share", narf::util::dirName("\\\\server\\share\\"));
	ASSERT_EQ("\\\\server\\share", narf::util::dirName("\\\\server\\share\\dir"));
	ASSERT_EQ("\\\\server\\share", narf::util::dirName("\\\\server\\share\\dir\\"));
	ASSERT_EQ("\\\\server\\share\\dir", narf::util::dirName("\\\\server\\share\\dir\\dir2"));
}

TEST(PATH, baseName) {
	ASSERT_EQ("C:\\", narf::util::baseName("C:\\"));
	ASSERT_EQ("foo", narf::util::baseName("foo"));
	ASSERT_EQ("foo", narf::util::baseName("C:\\foo"));
	ASSERT_EQ("foo", narf::util::baseName("C:\\foo\\"));
	ASSERT_EQ("bar", narf::util::baseName("C:\\foo\\bar"));
}

#endif
