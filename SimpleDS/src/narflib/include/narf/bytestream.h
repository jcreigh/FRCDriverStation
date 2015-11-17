/*
 * NarfBlock bytestream writer and reader
 *
 * Copyright (c) 2014 Daniel Verkamp, Jessica Creighton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NARF_BYTESTREAM_H
#define NARF_BYTESTREAM_H

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>

#define LE narf::ByteStream::LITTLE
#define BE narf::ByteStream::BIG

namespace narf {

class ByteStream {
public:
	ByteStream();
	ByteStream(size_t size);
	ByteStream(std::string data);
	ByteStream(const void* data, size_t size);

	~ByteStream();

	enum Type {
		U8, U16, U32, U64, I8, I16, I32, I64, FLOAT, DOUBLE
	};

	enum Endian {
		LITTLE, BIG, DEFAULT
	};

	void write(const std::vector<uint8_t> data);
	void write(const std::string data);
	void write(const void* data, size_t size);
	void write(const void* data, Type type, Endian endian = Endian::DEFAULT);
	void write(uint8_t v);
	void write(int8_t v);
	void write(uint16_t v, Endian endian = Endian::DEFAULT);
	void write(int16_t v, Endian endian = Endian::DEFAULT);
	void write(uint32_t v, Endian endian = Endian::DEFAULT);
	void write(int32_t v, Endian endian = Endian::DEFAULT);
	void write(uint64_t v, Endian endian = Endian::DEFAULT);
	void write(int64_t v, Endian endian = Endian::DEFAULT);
	void write(float v, Endian endian = Endian::DEFAULT);
	void write(double v, Endian endian = Endian::DEFAULT);
	void writeString(const std::string data, Type type, Endian endian = Endian::DEFAULT);
	void writeString(const void* data, size_t size, Type type, Endian endian = Endian::DEFAULT);

	bool read(void* v, Type type, Endian endian = Endian::DEFAULT);
	std::vector<uint8_t> read(size_t c);
	bool read(void* v, size_t c);
	std::vector<uint8_t> readString(Type type = Type::U16, Endian endian = Endian::DEFAULT);
	bool read(int8_t* v);
	bool read(uint8_t* v);
	uint8_t readU8();
	int8_t readI8();
	uint16_t readU16(Endian endian = Endian::DEFAULT);
	int16_t readI16(Endian endian = Endian::DEFAULT);
	uint32_t readU32(Endian endian = Endian::DEFAULT);
	int32_t readI32(Endian endian = Endian::DEFAULT);
	uint64_t readU64(Endian endian = Endian::DEFAULT);
	int64_t readI64(Endian endian = Endian::DEFAULT);
	float readFloat(Endian endian = Endian::DEFAULT);
	double readDouble(Endian endian = Endian::DEFAULT);
	bool read(uint16_t* v, Endian endian = Endian::DEFAULT);
	bool read(int16_t* v, Endian endian = Endian::DEFAULT);
	bool read(uint32_t* v, Endian endian = Endian::DEFAULT);
	bool read(int32_t* v, Endian endian = Endian::DEFAULT);
	bool read(uint64_t* v, Endian endian = Endian::DEFAULT);
	bool read(int64_t* v, Endian endian = Endian::DEFAULT);
	bool read(float* v, Endian endian = Endian::DEFAULT);
	bool read(double* v, Endian endian = Endian::DEFAULT);

	Endian getDefault() { return default_; }
	void setDefault(Endian endian) { default_ = endian; }

	void seek(size_t newPos);
	size_t tell();
	void skip(size_t c);
	void clear();
	size_t bytesLeft() { return size() - pos; }

	bool overran();

	void* data() { return data_.data(); }
	std::vector<uint8_t> vec() { return data_; }
	std::string str() { return std::string(data_.begin(), data_.end()); }
	size_t size() { return data_.size(); }

private:
	size_t pos;
	bool overran_;
	Endian default_;
	std::vector<uint8_t> data_;

	static uint8_t typeSize(Type type);
};

} // namespace narf

#endif // NARF_BYTESTREAM_H
