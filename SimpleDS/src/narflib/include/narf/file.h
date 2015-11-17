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

#ifndef NARF_FILE_H
#define NARF_FILE_H

#include <string>

namespace narf {

// Represents a full file loaded into memory at once.
// This is intended to be used for small files - at the limit, the file size has to at least fit in size_t.
class MemoryFile {
public:
	MemoryFile();
	~MemoryFile();

	bool read(const char* filename);
	bool read(const std::string& filename) { return read(filename.c_str()); }

	bool write(const char* filename) const;
	bool write(const std::string& filename) const { return write(filename.c_str()); }

	bool resize(size_t newSize);

	bool setData(const void* data, size_t size);
	bool setData(const std::string data) { return setData(data.c_str(), data.size()); }

	std::string str() const { return std::string((const char*)data, size); }

	void* data;
	size_t size;
};

} // namespace narf

#endif // NARF_FILE_H
