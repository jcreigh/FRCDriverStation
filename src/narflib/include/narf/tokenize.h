#ifndef NARF_UTIL_TOKENIZE_H
#define NARF_UTIL_TOKENIZE_H

#include <stdlib.h>
#include <string>
#include <vector>

namespace narf {
	namespace util {
		std::vector<std::string>& tokenize(const std::string &input, char delimeter, std::vector<std::string>& tokens);
		std::vector<std::string> tokenize(const std::string &input, char delimeter);
		std::string join(const std::vector<std::string> &input, std::string delimeter);
	}
}

#endif
