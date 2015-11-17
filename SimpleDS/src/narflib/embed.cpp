/*
 * Embedded binary accessors
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

#include "narf/embed.h"

#include <zlib.h>
#include <stdio.h>

namespace narf {
namespace embed {

bool uncompress(void* dst, size_t dstSize, const void* src, size_t srcSize) {
	z_stream st;

	st.next_in = static_cast<Bytef*>(const_cast<void*>(src));
	st.avail_in = static_cast<uInt>(srcSize);

	st.next_out = static_cast<Bytef*>(dst);
	st.avail_out = static_cast<uInt>(dstSize);

	st.zalloc = nullptr;
	st.zfree = nullptr;

	if (::inflateInit2(&st, 32) != Z_OK) { // 32 = autodetect zlib/gzip
		return false;
	}

	if (::inflate(&st, Z_FINISH) != Z_STREAM_END) {
		return false;
	}

	if (st.total_out != dstSize) {
		return false;
	}

	return true;
}

}} // namespace narf::embed
