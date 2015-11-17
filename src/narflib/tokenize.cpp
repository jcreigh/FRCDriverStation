#include "narf/tokenize.h"
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>

std::vector<std::string>& narf::util::tokenize(const std::string &input, char delimeter, std::vector<std::string>& tokens) {
	std::stringstream stream(input);
	std::string token;
	while (std::getline(stream, token, delimeter)) {
		tokens.push_back(token);
	}
	return tokens;
}

std::vector<std::string> narf::util::tokenize(const std::string &input, char delimeter) {
	std::vector<std::string> tokens;
	narf::util::tokenize(input, delimeter, tokens);
	return tokens;
}

std::string narf::util::join(const std::vector<std::string> &input, std::string delimeter) {
	std::string output;
	for (const auto& i : input) {
		if (&i != &input[0]) {
			output += delimeter;
		}
		output += i;
	}
	return output;
}
