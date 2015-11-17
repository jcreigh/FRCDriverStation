/*
 * File I/O utilities
 *
 * Copyright (c) 2015 Daniel Verkamp
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <algorithm>

#include "narf/file.h"
#include "narf/utf.h"


#ifdef _WIN32
static FILE* fopenUTF8(const char* filename, const char* mode) {
	std::wstring filenameW, modeW;
	narf::toUTF16(filename, filenameW);
	narf::toUTF16(mode, modeW);
	return _wfopen(filenameW.c_str(), modeW.c_str());
}
#else
#define fopenUTF8 fopen
#endif


narf::MemoryFile::MemoryFile() : data(nullptr), size(0) {
}


narf::MemoryFile::~MemoryFile() {
	resize(0);
}


bool narf::MemoryFile::resize(size_t newSize) {
	void* newData = newSize ? malloc(newSize) : nullptr;
	if (newSize == 0 || newData != nullptr) {
		if (data) {
			memcpy(newData, data, std::min(size, newSize));
			free(data);
		}
		data = newData;
		size = newSize;
	}
	return true;
}

bool narf::MemoryFile::setData(const void* newData, size_t newSize) {
	if (data) {
		free(data);
		size = 0;
	}
	if (newSize == 0) {
		data = nullptr;
		return true;
	}
	data = malloc(newSize);
	if (data == nullptr) {
		return false;
	}
	memcpy(data, newData, newSize);
	size = newSize;
	return true;
}

bool narf::MemoryFile::read(const char* filename) {
	FILE* fp = fopenUTF8(filename, "rb");
	if (!fp) {
		return false;
	}

	fseek(fp, 0, SEEK_END);
	size_t fileSize = static_cast<size_t>(ftell(fp)); // TODO: use ftello/64-bit stuff when available
	fseek(fp, 0, SEEK_SET);

	if (fileSize > SIZE_MAX) {
		fclose(fp);
		return false;
	}

	size_t fileSizeSzT = (size_t)fileSize;
	void* newData = malloc(fileSizeSzT);
	if (!newData) {
		fclose(fp);
		return false;
	}

	if (fread(newData, 1, fileSizeSzT, fp) != fileSizeSzT) {
		fclose(fp);
		free(newData);
		return false;
	}
	fclose(fp);
	if (data) {
		free(data);
	}
	data = newData;
	size = fileSizeSzT;
	return true;
}


bool narf::MemoryFile::write(const char* filename) const {
	FILE* fp = fopenUTF8(filename, "wb");
	if (!fp) {
		return false;
	}
	size_t written = fwrite(data, sizeof(char), size, fp);
	if (written != size) {
		fclose(fp);
		return false;
	}
	fclose(fp);
	return true;
}
