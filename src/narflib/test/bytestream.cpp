#include "narf/bytestream.h"
#include <stdint.h>
#include <gtest/gtest.h>

TEST(ByteStream, U8) {
	narf::ByteStream bs;
	bs.write((uint8_t)57);
	bs.write((uint8_t)137);
	bs.seek(0);
	ASSERT_EQ(57, bs.readU8());
	uint8_t t;
	bs.read(&t);
	ASSERT_EQ(137, t);
}

TEST(ByteStream, I8) {
	narf::ByteStream bs;
	bs.write((int8_t)57);
	bs.write((int8_t)-100);
	bs.seek(0);
	ASSERT_EQ(57, bs.readI8());
	int8_t t;
	bs.read(&t);
	ASSERT_EQ(-100, t);
}

TEST(ByteStream, U16) {
	narf::ByteStream bs;
	bs.write((uint16_t)0xa0b1, LE);
	bs.write((uint16_t)0xc2d3, BE);
	bs.seek(0);
	ASSERT_EQ(0xb1a0, bs.readU16(BE));
	uint16_t t;
	bs.read(&t, LE);
	ASSERT_EQ(0xd3c2, t);
}

TEST(ByteStream, I16) {
	narf::ByteStream bs;
	bs.write((int16_t)-7340, LE);
	bs.write((int16_t)0x5061, BE);
	bs.seek(0);
	ASSERT_EQ(21731, bs.readI16(BE));
	int16_t t;
	bs.read(&t, LE);
	ASSERT_EQ(0x6150, t);
}

TEST(ByteStream, U32) {
	narf::ByteStream bs;
	bs.write((uint32_t)0xa0b1c2d3, LE);
	bs.write((uint32_t)0xe4f50617, BE);
	bs.seek(0);
	ASSERT_EQ(0xd3c2b1a0, bs.readU32(BE));
	uint32_t t;
	bs.read(&t, LE);
	ASSERT_EQ(0x1706f5e4, t);
}

TEST(ByteStream, I32) {
	narf::ByteStream bs;
	bs.write((int32_t)-123456, LE);
	bs.write((int32_t)78910, BE);
	bs.seek(0);
	ASSERT_EQ(-1071776001, bs.readI32(BE));
	int32_t t;
	bs.read(&t, LE);
	ASSERT_EQ(0x3e340100, t);
}

TEST(ByteStream, U64) {
	narf::ByteStream bs;
	bs.write((uint64_t)0xa0b1c2d3e4f50617, LE);
	bs.write((uint64_t)0x0102030405060708, BE);
	bs.seek(0);
	ASSERT_EQ((uint64_t)0x1706f5e4d3c2b1a0, bs.readU64(BE));
	uint64_t t;
	bs.read(&t, LE);
	ASSERT_EQ((uint64_t)0x0807060504030201, t);
}

TEST(ByteStream, I64) {
	narf::ByteStream bs;
	bs.write((int64_t)-1234567890, LE);
	bs.write((int64_t)0x01020304050607ff, BE);
	bs.seek(0);
	ASSERT_EQ((int64_t)3385978729552412671, bs.readI64(BE));
	int64_t t;
	bs.read(&t, LE);
	ASSERT_EQ((int64_t)-70080650589044223, t);
}

TEST(ByteStream, Float) {
	narf::ByteStream bs;
	bs.write(-1234.5678f, LE);
	bs.write(1234.5f, LE);
	bs.seek(0);
	ASSERT_EQ(0x2b529ac4, bs.readU32(BE));
	float t;
	bs.read(&t, LE);
	ASSERT_FLOAT_EQ(1234.5f, t);
}

TEST(ByteStream, Double) {
	narf::ByteStream bs;
	bs.write(-123456.78901, LE);
	bs.write(1234.5, LE);
	bs.seek(0);
	ASSERT_EQ((uint64_t)0x23f3c89f0c24fec0, bs.readU64(BE));
	double t;
	bs.read(&t, LE);
	ASSERT_DOUBLE_EQ(1234.5, t);
}

TEST(ByteStream, writeVec) {
	narf::ByteStream bs;
	std::vector<uint8_t> vec;
	vec.push_back(0xa0);
	vec.push_back(0xb1);
	vec.push_back(0xc2);
	vec.push_back(0xd3);
	bs.write(vec);
	bs.seek(0);
	ASSERT_EQ(0xa0b1c2d3, bs.readU32(BE));
}

TEST(ByteStream, readVec) {
	narf::ByteStream bs;
	bs.write((uint64_t)0x0102030405060708, LE);
	bs.write(0x090a0b0c, LE);
	bs.seek(0);
	auto vec = bs.read(8);
	ASSERT_EQ(0x08, vec[0]);
	ASSERT_EQ(0x07, vec[1]);
	ASSERT_EQ(0x06, vec[2]);
	ASSERT_EQ(0x05, vec[3]);
	ASSERT_EQ(0x04, vec[4]);
	ASSERT_EQ(0x03, vec[5]);
	ASSERT_EQ(0x02, vec[6]);
	ASSERT_EQ(0x01, vec[7]);
}

TEST(ByteStream, str) {
	std::string a = "abcdefghijklmnop";
	narf::ByteStream bs;
	bs.write(a);
	ASSERT_EQ(a, bs.str());
}

TEST(ByteStream, overran) {
	narf::ByteStream bs;
	uint8_t t;
	ASSERT_FALSE(bs.read(&t));
	t = bs.readU8();
	ASSERT_TRUE(bs.overran());
}

TEST(ByteStream, size) {
	narf::ByteStream bs(7);
	ASSERT_EQ(7, bs.size());
}

TEST(ByteStream, skip) {
	narf::ByteStream bs;
	bs.write(std::string("abcd"));
	bs.seek(0);
	bs.skip(2);
	ASSERT_EQ('c', bs.readU8());
}

TEST(ByteStream, tell) {
	narf::ByteStream bs;
	bs.write(std::string("abcd"));
	bs.seek(0);
	bs.skip(2);
	ASSERT_EQ(2, bs.tell());
}

TEST(ByteStream, bytesLeft) {
	narf::ByteStream bs;
	bs.write(std::string("abcd"));
	bs.seek(0);
	bs.skip(1);
	ASSERT_EQ(3, bs.bytesLeft());
}

TEST(ByteStream, clear) {
	narf::ByteStream bs;
	bs.write(std::string("abcd"));
	bs.clear();
	ASSERT_EQ(0, bs.size());
}

TEST(ByteStream, setDefault) {
	narf::ByteStream bs;
	bs.setDefault(BE);
	bs.write((uint16_t)0xa1b2);
	bs.seek(0);
	bs.setDefault(LE);
	ASSERT_EQ(0xb2a1, bs.readU16());
}

TEST(ByteStream, ctorData) {
	const char* data = "abc";
	narf::ByteStream bs(data, 3);
	bs.skip(1);
	ASSERT_EQ('b', bs.readU8());
}

TEST(ByteStream, ctorString) {
	std::string data = "abc";
	narf::ByteStream bs(data);
	bs.skip(1);
	ASSERT_EQ('b', bs.readU8());
}

TEST(ByteStream, writeString) {
	narf::ByteStream bs;
	bs.writeString(std::string("abcd"), narf::ByteStream::Type::U16, LE);
	bs.writeString(std::string("efgh"), narf::ByteStream::Type::U16, LE);
	bs.seek(0);
	auto vec = bs.readString(narf::ByteStream::Type::U16, LE);
	auto s = std::string(vec.begin(), vec.end());
	ASSERT_EQ(std::string("abcd"), s);
	ASSERT_EQ(12, bs.size());
}
