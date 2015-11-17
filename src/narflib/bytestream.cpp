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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "narf/bytestream.h"

narf::ByteStream::ByteStream() : pos(0) { }

narf::ByteStream::ByteStream(size_t size) : pos(0) {
	data_.resize(size);
}

narf::ByteStream::ByteStream(std::string data) : ByteStream(data.c_str(), data.size()) {
}

narf::ByteStream::ByteStream(const void* data, size_t size) : pos(0) {
	if (data != nullptr) {
		auto v = static_cast<const uint8_t*>(data);
		data_ = std::vector<uint8_t>(v, v + size);
	} else {
		data_.reserve(size);
	}
}

narf::ByteStream::~ByteStream() { }

uint8_t narf::ByteStream::typeSize(ByteStream::Type type) {
	if (type == ByteStream::Type::U8 || type == ByteStream::Type::I8) {
		return 1;
	} else if (type == ByteStream::Type::U16 || type == ByteStream::Type::I16) {
		return 2;
	} else if (type == ByteStream::Type::U32 || type == ByteStream::Type::I32 ||
	           type == ByteStream::Type::FLOAT) {
		return 4;
	} else if (type == ByteStream::Type::U64 || type == ByteStream::Type::I64 ||
	           type == ByteStream::Type::DOUBLE) {
		return 8;
	}
	return 1;
}

void narf::ByteStream::write(const std::vector<uint8_t> data) {
	write(data.data(), data.size());
}

void narf::ByteStream::write(const std::string data) {
	write(data.data(), data.size());
}

void narf::ByteStream::write(const void* data, size_t size) {
	if (pos + size > data_.size()) {
		data_.resize(pos + size, 0);
	}

	memcpy(&data_[pos], data, size);
	pos += size;
}

void narf::ByteStream::write(const void* data, Type type, Endian endian /*= Endian::DEFAULT*/) {
	auto v = static_cast<const uint8_t*>(data);
	uint8_t size = typeSize(type);

	if (pos + size > data_.size()) {
		data_.resize(pos + size, 0);
	}

	if (endian == Endian::DEFAULT) {
		endian = default_;
	}

	for (int i = 0; i < size; i++) {
		data_[pos + i] = v[(endian == Endian::BIG ? (size - 1 - i) : i)];
	}
	pos += size;

}

void narf::ByteStream::write(uint8_t v) {
	write(&v, ByteStream::Type::U8);
}

void narf::ByteStream::write(int8_t v) {
	write(&v, ByteStream::Type::I8);
}

void narf::ByteStream::write(uint16_t v, Endian endian /*= Endian::DEFAULT*/) {
	write(&v, ByteStream::Type::U16, endian);
}

void narf::ByteStream::write(int16_t v, Endian endian /*= Endian::DEFAULT*/) {
	write(&v, ByteStream::Type::I16, endian);
}

void narf::ByteStream::write(uint32_t v, Endian endian /*= Endian::DEFAULT*/) {
	write(&v, ByteStream::Type::U32, endian);
}

void narf::ByteStream::write(int32_t v, Endian endian /*= Endian::DEFAULT*/) {
	write(&v, ByteStream::Type::I32, endian);
}

void narf::ByteStream::write(uint64_t v, Endian endian /*= Endian::DEFAULT*/) {
	write(&v, ByteStream::Type::U64, endian);
}

void narf::ByteStream::write(int64_t v, Endian endian /*= Endian::DEFAULT*/) {
	write(&v, ByteStream::Type::I64, endian);
}

void narf::ByteStream::write(float v, Endian endian /*= Endian::DEFAULT*/) {
	uint32_t i;
	static_assert(sizeof(v) == sizeof(i), "float should be 32 bits");
	write(&v, ByteStream::Type::FLOAT, endian);
}

void narf::ByteStream::write(double v, Endian endian /*= Endian::DEFAULT*/) {
	uint64_t i;
	static_assert(sizeof(v) == sizeof(i), "double should be 64 bits");
	write(&v, ByteStream::Type::DOUBLE, endian);
}

void narf::ByteStream::writeString(const std::string data, Type type, Endian endian /*= Endian::DEFAULT*/) {
	writeString((const void*)data.c_str(), data.size(), type, endian);
}

void narf::ByteStream::writeString(const void* data, size_t size, Type type, Endian endian /*= Endian::DEFAULT*/) {
	write(&size, type, endian);
	write(data, size);
}

bool narf::ByteStream::read(void* v, narf::ByteStream::Type type, Endian endian) {
	assert(v != nullptr);
	uint8_t size = typeSize(type);
	if (pos + size > this->size()) {
		overran_ = true;
		return false;
	}

	if (endian == Endian::DEFAULT) {
		endian = default_;
	}

	auto vp = static_cast<uint8_t*>(v);
	for (int i = 0; i < size; i++) {
		vp[i] = data_[pos + (endian == ByteStream::Endian::BIG ? (size - 1 - i) : i)];
	}
	pos += size;
	overran_ = false;
	return true;
}

bool narf::ByteStream::read(void* v, size_t c) {
	memcpy(v, (const void*)read(c).data(), c);
	return !overran_;
}

std::vector<uint8_t> narf::ByteStream::read(size_t c) {
	if (overran_ || pos + c > size()) {
		overran_ = true;
		c = size() - pos;
	} else {
		overran_ = false;
	}
	auto vec = std::vector<uint8_t>(data_.begin() + pos, data_.begin() + pos + c);
	pos += c;
	return vec;
}

std::vector<uint8_t> narf::ByteStream::readString(Type type /*= Type::U16*/, Endian endian /*= Endian::DEFAULT*/) {
	size_t size = 0;
	read(&size, type, endian);
	return read(size);
}

bool narf::ByteStream::read(uint8_t* v) {
	return read(v, Type::U8);
}

uint8_t narf::ByteStream::readU8() {
	uint8_t v = 0;
	read(&v, Type::U8);
	return v;
}

bool narf::ByteStream::read(int8_t* v) {
	return read(v, Type::I8);
}

int8_t narf::ByteStream::readI8() {
	int8_t v = 0;
	read(&v, Type::I8);
	return v;
}

uint16_t narf::ByteStream::readU16(Endian endian /*= Endian::DEFAULT*/) {
	uint16_t v = 0;
	read(&v, Type::U16, endian);
	return v;
}

int16_t narf::ByteStream::readI16(Endian endian /*= Endian::DEFAULT*/) {
	int16_t v = 0;
	read(&v, Type::I16, endian);
	return v;
}

uint32_t narf::ByteStream::readU32(Endian endian /*= Endian::DEFAULT*/) {
	uint32_t v = 0;
	read(&v, Type::U32, endian);
	return v;
}

int32_t narf::ByteStream::readI32(Endian endian /*= Endian::DEFAULT*/) {
	int32_t v = 0;
	read(&v, Type::I32, endian);
	return v;
}

uint64_t narf::ByteStream::readU64(Endian endian /*= Endian::DEFAULT*/) {
	uint64_t v = 0;
	read(&v, Type::U64, endian);
	return v;
}

int64_t narf::ByteStream::readI64(Endian endian /*= Endian::DEFAULT*/) {
	int64_t v = 0;
	read(&v, Type::I64, endian);
	return v;
}

float narf::ByteStream::readFloat(Endian endian /*= Endian::DEFAULT*/) {
	float v = 0.0f;
	read(&v, Type::U16, endian);
	return v;
}

double narf::ByteStream::readDouble(Endian endian /*= Endian::DEFAULT*/) {
	double v = 0.0;
	read(&v, Type::I16, endian);
	return v;
}

bool narf::ByteStream::read(uint16_t* v, Endian endian /*= Endian::DEFAULT*/) {
	return read(v, Type::U16, endian);
}

bool narf::ByteStream::read(int16_t* v, Endian endian /*= Endian::DEFAULT*/) {
	return read(v, Type::I16, endian);
}

bool narf::ByteStream::read(uint32_t* v, Endian endian /*= Endian::DEFAULT*/) {
	return read(v, Type::U32, endian);
}

bool narf::ByteStream::read(int32_t* v, Endian endian /*= Endian::DEFAULT*/) {
	return read(v, Type::I32, endian);
}

bool narf::ByteStream::read(uint64_t* v, Endian endian /*= Endian::DEFAULT*/) {
	return read(v, Type::U64, endian);
}

bool narf::ByteStream::read(int64_t* v, Endian endian /*= Endian::DEFAULT*/) {
	return read(v, Type::I64, endian);
}

bool narf::ByteStream::read(float* v, Endian endian /*= Endian::DEFAULT*/) {
	return read(v, Type::FLOAT, endian);
}

bool narf::ByteStream::read(double* v, Endian endian /*= Endian::DEFAULT*/) {
	return read(v, Type::DOUBLE, endian);
}

void narf::ByteStream::seek(size_t newPos) {
	if (pos > size()) {
		pos = size();
	} else {
		pos = newPos;
	}
}

size_t narf::ByteStream::tell() {
	return pos;
}

void narf::ByteStream::skip(size_t c) {
	pos = (pos + c > size()) ? size() : (pos + c);
}

void narf::ByteStream::clear() {
	data_.clear();
	pos = 0;
}

bool narf::ByteStream::overran() {
	bool v = overran_;
	overran_ = false;
	return v;
}
