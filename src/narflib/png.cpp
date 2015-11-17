/*
 * PNG file loader
 *
 * Copyright (c) 2014 Daniel Verkamp
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

#include "narf/image.h"

#include "narf/console.h"

#include <assert.h>
#include <png.h>
#include <zlib.h>
#include <stdint.h>
#include <stdlib.h>


narf::Image::Image(uint32_t width, uint32_t height) : width_(width), height_(height) {
	data_ = malloc(width * height * 4); // TODO: handle failure
}

narf::Image::~Image() {
	free(data_);
}


struct PngReadCtx {
	const uint8_t* data;
	size_t size;
};

void pngErrorCallback(png_structp png, png_const_charp errorMsg) {
	narf::console->println("libpng error: " + std::string(errorMsg));
}

void pngWarningCallback(png_structp png, png_const_charp warnMsg) {
	narf::console->println("libpng warning: " + std::string(warnMsg));
}

void pngReadCallback(png_structp png, png_bytep data, png_size_t length) {
	auto ctx = static_cast<PngReadCtx*>(png_get_io_ptr(png));
	if (length > ctx->size) {
		png_error(png, "Read Error: out of data");
		return;
	}

	memcpy(data, ctx->data, length);
	ctx->size -= length;
	ctx->data += length;
}


// load PNG file into 8-bit RGBA bitmap (GL_RGBA8UI)
narf::Image* narf::loadPNG(const void* data, size_t size) {
	narf::console->println("size = " + std::to_string(size));
	if (size < 8 || png_sig_cmp((png_bytep)data, 0, 8)) {
		narf::console->println("loadPNG: signature does not match");
		return nullptr;
	}

	png_structp png = nullptr;
	png_infop info = nullptr;
	png_bytep* rows = nullptr;
	narf::Image* image = nullptr;

	PngReadCtx readCtx = {static_cast<const uint8_t*>(data), size};

	png_uint_32 width, height;
	int bitDepth, colorType;

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, pngErrorCallback, pngWarningCallback);
	if (!png) {
		narf::console->println("png_create_read_struct failed");
		goto cleanup;
	}

	info = png_create_info_struct(png);
	if (!info) {
		narf::console->println("png_create_info_struct failed");
		goto cleanup;
	}

	if (setjmp(png_jmpbuf(png))) {
		narf::console->println("libpng returned via setjmp (error)");
		if (image) {
			delete image;
			image = nullptr;
		}
		goto cleanup;
	}

	png_set_read_fn(png, &readCtx, pngReadCallback);

	// TODO: set CRC action (ignore errors on non-required chunks?)
	// TODO: set alpha and gamma handling

	png_read_info(png, info);
	png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr);

	if (colorType == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png);
	}

	if (png_get_valid(png, info, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png);
	}

	if (bitDepth < 8) {
		if (colorType == PNG_COLOR_TYPE_GRAY) {
			png_set_expand_gray_1_2_4_to_8(png);
		} else {
			png_set_packing(png);
		}
	}

	if (bitDepth == 16) {
#if PNG_LIBPNG_VER >= 10504
		png_set_scale_16(png);
#else
		png_set_strip_16(png);
#endif
	}

	// TODO: handle background?

	// for simplicity, Image always has an alpha channel, so fill it with 0xFF (fully opaque) for RGB images
	if (!(colorType & PNG_COLOR_MASK_ALPHA)) {
		png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
	}

	// apply transformations to info
	png_read_update_info(png, info);
	png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr);

	image = new narf::Image(width, height);
	if (!image) {
		narf::console->println("new Image() failed");
		goto cleanup;
	}

	rows = new png_bytep[image->height()];
	if (!rows) {
		narf::console->println("rows allocation failed");
		delete image;
		image = nullptr;
		goto cleanup;
	}

	{
		auto stride = png_get_rowbytes(png, info);
		png_bytep offs = static_cast<png_bytep>(image->data());
		for (uint32_t y = 0; y < image->height(); y++) {
			rows[y] = offs;
			offs += stride;
		}
	}

	png_read_image(png, rows);
	png_read_end(png, nullptr);

cleanup:
	png_destroy_read_struct(&png, &info, nullptr);
	if (rows) {
		delete[] rows;
	}
	return image;
}
