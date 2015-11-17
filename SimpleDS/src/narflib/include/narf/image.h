#ifndef NARF_PNG_H
#define NARF_PNG_H

#include <stdint.h>
#include <cstddef>
#include <cstring>

namespace narf {
	class Image {
	public:
		Image(uint32_t w, uint32_t h);
		Image(const void* data, size_t size);
		~Image();

		void* data() { return data_; }
		uint32_t width() const { return width_; }
		uint32_t height() const { return height_; }

	private:
		void* data_;
		uint32_t width_;
		uint32_t height_;
	};

	// TODO: use a smart pointer of some kind
	Image* loadPNG(const void* data, size_t size);
} // namespace narf

#endif
