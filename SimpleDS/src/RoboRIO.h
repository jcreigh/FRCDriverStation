/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _ROBORIO_H_
#define _ROBORIO_H_

#include "enums.h"
#include "narf/bytestream.h"
#include <chrono>
#include <string>
#include <vector>
#include <cstring>
#include <arpa/inet.h>


class RoboRIO {
	private:
		std::chrono::system_clock::time_point lastPacket;
		void reset();
		void check();

	public:
		struct Packet {
			uint16_t seqNum;
			struct Control {
				uint8_t mode : 2;
				bool enabled : 1;
				bool         : 4;
				bool estop   : 1;
				bool         : 5;
				bool code    : 1;
				bool         : 2;
			} control;
			uint8_t battery[2];
		};

		struct Usage {
			uint32_t disk;
			uint32_t ram;
		};

		struct CAN {
			uint8_t util;
			uint8_t busOff;
			uint8_t txFull;
			uint8_t receive;
			uint8_t transmit;
		};

		struct Output {
			uint32_t outputs;
			uint16_t rumbleLeft;
			uint16_t rumbleRight;
		};

		float cpus[2];
		Usage usage;
		CAN can;
		Output outputs[6];
		uint8_t jsOutIdx;
		Packet packet;
		void parsePacket(std::string data);
		size_t parseExtended(std::string data);
		RoboRIO();
		bool getEnable();
		Mode getMode();
		bool getCode();
		bool getEStop();
		float getBattery();
};

#endif /* _ROBORIO_H_ */
