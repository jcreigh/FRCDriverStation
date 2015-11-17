#include <stdio.h>

#include <gtest/gtest.h>
#include "narf/ini.h"


TEST(INI, BasicRead) {
	std::string inData = " foo  = \"1\"     \n  [bar] ; baz \n\tqux = quux\n\nquz = true\n[mar]\n\n\nqux = 0.5\nquz = 0.75\n";
	auto ini = new narf::INI::File();
	ini->load(inData);
	ASSERT_EQ(1, ini->getInt32("foo"));
	ASSERT_EQ("quux", ini->getString("bar.qux"));
	ASSERT_EQ(true, ini->getBool("bar.quz"));
	ASSERT_EQ(0.5, ini->getFloat("mar.qux"));
	ASSERT_EQ(0.75, ini->getDouble("mar.quz"));
	delete ini;
}

TEST(INI, Save) {
	auto ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("foo = 1\n"));
	ASSERT_EQ(std::string("foo = 1\n"), ini->save());
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("foo = 1"));
	ASSERT_EQ(std::string("foo = 1"), ini->save());
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load(" foo  = \"1\"     \n  [bar] ; baz \n\tqux = quux\n\nquz = true\n[mar]\n\n\nqux = 0.5\nquz = 0.75\n"));
	ini->setInt32("foo", 2);
	ASSERT_EQ(std::string(" foo  = 2     \n  [bar] ; baz \n\tqux = quux\n\nquz = true\n[mar]\n\n\nqux = 0.5\nquz = 0.75\n"), ini->save());
	delete ini;
}

TEST(INI, Int32) {
	narf::INI::File ini;
	ini.load("foo = 501");
	ASSERT_EQ(501, ini.getInt32("foo"));
	ini.setString("foo", "106232");
	ASSERT_EQ(106232, ini.getInt32("foo"));
	ini.setInt32("foo", 123456);
	ASSERT_EQ(123456, ini.getInt32("foo"));
}

TEST(INI, String) {
	narf::INI::File ini;
	ini.load("foo = beep");
	ASSERT_EQ("beep", ini.getString("foo"));
	ini.setInt32("foo", 123);
	ASSERT_EQ("123", ini.getString("foo"));
	ini.setFloat("foo", 0.5);
	ASSERT_EQ("0.5", ini.getString("foo"));
	ini.setDouble("foo", 0.75);
	ASSERT_EQ("0.75", ini.getString("foo"));
	ini.setBool("foo", true);
	ASSERT_EQ("true", ini.getString("foo"));
}

TEST(INI, Comments) {
	auto ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("; comment\nfoo = beep"));
	ASSERT_EQ("beep", ini->getString("foo"));
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("[bar] ; bez\nfoo = beep"));
	ASSERT_EQ("beep", ini->getString("bar.foo"));
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("[baz]\n; bez\nfoo = beep"));
	ASSERT_EQ("beep", ini->getString("baz.foo"));
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("[qux]\nfoo = \"beep \"  ; Bez"));
	ASSERT_EQ("beep ", ini->getString("qux.foo"));
	delete ini; ini = new narf::INI::File();
	ASSERT_FALSE(ini->load("[qux]\nfoo = \"beep \"  bex ; Bez"));
	ASSERT_EQ("beep ", ini->getString("qux.foo"));
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("[qux]\nfoo = beep ; Bez"));
	ASSERT_EQ("beep", ini->getString("qux.foo"));
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("[qux]\nfoo = beep honk    ; Bez"));
	ASSERT_EQ("beep honk", ini->getString("qux.foo"));
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("[qux]\nfoo = \"beep;;honk\""));
	ASSERT_EQ("beep;;honk", ini->getString("qux.foo"));
	delete ini; ini = new narf::INI::File();
	ASSERT_TRUE(ini->load("[qux]\nfoo = beep\\;\\;honk"));
	ASSERT_EQ("beep;;honk", ini->getString("qux.foo"));
	delete ini;
}

TEST(INI, Remove) {
	auto ini = new narf::INI::File();
	ini->load("foo = beep\n[bar]\nbaz = 1\nqux = 2");
	ASSERT_FALSE(ini->remove("meep"));
	ASSERT_TRUE(ini->remove("foo"));
	std::string expectedOut = "[bar]\nbaz = 1\nqux = 2";
	ASSERT_EQ(expectedOut, ini->save());
	delete ini; ini = new narf::INI::File(); ini->load("foo = beep\n[bar]\nbaz = 1\nqux = 2");
	ASSERT_TRUE(ini->remove("bar.baz"));
	ASSERT_EQ(std::string("foo = beep\n[bar]\nqux = 2"), ini->save());
	delete ini; ini = new narf::INI::File(); ini->load("foo = beep\n[bar]\nbaz = 1\nqux = 2");
	ASSERT_TRUE(ini->remove("bar.qux"));
	ASSERT_EQ(std::string("foo = beep\n[bar]\nbaz = 1\n"), ini->save());
	delete ini; ini = new narf::INI::File(); ini->load("foo = beep\n[bar]\nqux = 3\nbaz = 1\nqux = 2");
	ASSERT_TRUE(ini->remove("bar.qux"));
	ASSERT_EQ(std::string("foo = beep\n[bar]\nbaz = 1\n"), ini->save());
	delete ini; ini = new narf::INI::File(); ini->load("foo = beep\n[bar]\nbaz = 1 ; comment\nqux = 2");
	ASSERT_TRUE(ini->remove("bar.baz"));
	ASSERT_EQ(std::string("foo = beep\n[bar]\nqux = 2"), ini->save());
	ASSERT_TRUE(ini->remove("foo"));
	ASSERT_EQ(std::string("[bar]\nqux = 2"), ini->save());
	ASSERT_TRUE(ini->remove("bar.qux"));
	ASSERT_EQ(std::string("[bar]\n"), ini->save());
	delete ini;
}

TEST(INI, UpdateLine) {
	auto ini = new narf::INI::File();
	ini->load("[bar]\n baz = \n");
	ini->setString("bar.baz", "foobarbaz");
	ASSERT_EQ("[bar]\n baz = foobarbaz\n", ini->save());
	ini->setString("bar.baz", "foo");
	ASSERT_EQ("[bar]\n baz = foo\n", ini->save());

}
