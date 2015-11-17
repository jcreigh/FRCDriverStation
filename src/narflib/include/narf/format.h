#ifndef NARF_FORMAT_H
#define NARF_FORMAT_H

#include <memory>
#include <sstream>
#include <stdarg.h>
#include <string.h>

namespace narf {
	namespace util {
		std::string format(const std::string fmt_str, ...);
	}
}

#endif /* NARF_FORMAT_H */
