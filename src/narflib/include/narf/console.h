#ifndef NARF_CONSOLE_H
#define NARF_CONSOLE_H

#include <string>

namespace narf {

	class Console;
	extern Console *console; // global console instance - TODO - hax

	class Console {
	public:
		virtual void println(const std::string &s) = 0;
		virtual std::string pollInput() = 0;

	//private: // TODO
		Console() {}
		virtual ~Console() {};
	};
}

#endif // NARF_CONSOLE_H
