#include "narf/net.h"
#include <gtest/gtest.h>

using namespace narf;

TEST(splitHostPortTest, Valid) {
	std::string host;
	uint16_t port;

	EXPECT_EQ(true, narf::net::splitHostPort("0.0.0.0", host, port));
	EXPECT_EQ("0.0.0.0", host);
	EXPECT_EQ(0, port);

	EXPECT_EQ(true, narf::net::splitHostPort("0.0.0.0:80", host, port));
	EXPECT_EQ("0.0.0.0", host);
	EXPECT_EQ(80, port);

	EXPECT_EQ(true, narf::net::splitHostPort("[::]", host, port));
	EXPECT_EQ("::", host);
	EXPECT_EQ(0, port);

	EXPECT_EQ(true, narf::net::splitHostPort("[1:2::3]", host, port));
	EXPECT_EQ("1:2::3", host);
	EXPECT_EQ(0, port);

	EXPECT_EQ(true, narf::net::splitHostPort("[1:2::3]:80", host, port));
	EXPECT_EQ("1:2::3", host);
	EXPECT_EQ(80, port);

	EXPECT_EQ(true, narf::net::splitHostPort("host", host, port));
	EXPECT_EQ("host", host);
	EXPECT_EQ(0, port);

	EXPECT_EQ(true, narf::net::splitHostPort("host:80", host, port));
	EXPECT_EQ("host", host);
	EXPECT_EQ(80, port);

	EXPECT_EQ(true, narf::net::splitHostPort("example.com", host, port));
	EXPECT_EQ("example.com", host);
	EXPECT_EQ(0, port);

	EXPECT_EQ(true, narf::net::splitHostPort("example.com:80", host, port));
	EXPECT_EQ("example.com", host);
	EXPECT_EQ(80, port);
}


TEST(splitHostPortTest, Invalid) {
	std::string host;
	uint16_t port;

	EXPECT_EQ(false, narf::net::splitHostPort("::", host, port)); // plain IPv6 without port - invalid for now, but should be unambiguous with IPv4 + port
	EXPECT_EQ(false, narf::net::splitHostPort("::1", host, port));
	EXPECT_EQ(false, narf::net::splitHostPort("1:2::3", host, port));
	EXPECT_EQ(false, narf::net::splitHostPort("[1:2::3]::80", host, port));
	EXPECT_EQ(false, narf::net::splitHostPort("1:[2:3]::4", host, port));
	EXPECT_EQ(false, narf::net::splitHostPort("1.2.3.4:[80]", host, port));
	EXPECT_EQ(false, narf::net::splitHostPort("host::80", host, port));
	EXPECT_EQ(false, narf::net::splitHostPort("example.com::80", host, port));
}
