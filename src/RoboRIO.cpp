/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "RoboRIO.h"

RoboRIO::RoboRIO() {
	reset();
}

void RoboRIO::reset() {
	memset((char*)&packet, 0, sizeof(packet));
}

void RoboRIO::check() {
	auto now = std::chrono::system_clock::now();
	if (now - lastPacket > std::chrono::seconds(1)) { // TODO: Make timeouts configurable?
		reset();
	}
}

void RoboRIO::parsePacket(std::string data) {
	auto reader = narf::ByteStream(data.c_str(), 8);
	packet.seqNum = reader.readU16(BE);
	reader.skip(1);
	reader.read(&packet.control, narf::ByteStream::Type::U16);
	reader.read(&packet.battery, 2);

	lastPacket = std::chrono::system_clock::now();

	size_t offset = 8;
	jsOutIdx = 0;

	if (data.size() > 8) {
		offset += parseExtended(data.substr(offset));
	} else {
		// This would have outputs if there were any, so clear everything
		memset(outputs, 0, sizeof(outputs));
	}
}

size_t RoboRIO::parseExtended(std::string data) {
	auto reader = narf::ByteStream(data.c_str(), data.size());
	uint8_t size = reader.readU8();
	uint8_t id = reader.readU8();
	if (id == 0x01) {
		Output* output = &(outputs[jsOutIdx++]);
		if (size == 1) {
			memset(&output, 0, sizeof(Output));
		} else {
			reader.read(&output->outputs, BE);
			reader.read(&output->rumbleLeft, BE);
			reader.read(&output->rumbleRight, BE);
		}
	} else if (id == 0x04) {
		reader.skip(3);
		reader.read(&usage.disk, BE);
	} else if (id == 0x05) {
		uint8_t count = reader.readU8();
		for (int i = 0; i < 2; i++) {
			reader.read(cpus + i, BE);
			reader.skip(12);
		}
	} else if (id == 0x06) {
		reader.skip(3);
		reader.read(&usage.ram, BE);
	} else if (id == 0x0e) {
		reader.skip(9);
		reader.read(&can.util);
		reader.read(&can.busOff);
		reader.read(&can.txFull);
		reader.read(&can.receive);
		reader.read(&can.transmit);
	}
	return size + 1;
}

bool RoboRIO::getEnable() {
	check();
	return packet.control.enabled;
}

Mode RoboRIO::getMode() {
	check();
	return (Mode)packet.control.mode;
}

bool RoboRIO::getCode() {
	check();
	return packet.control.code;
}

bool RoboRIO::getEStop() {
	check();
	return packet.control.estop;
}

float RoboRIO::getBattery() {
	check();
	return (float)(packet.battery[0]) + ((float)(packet.battery[1]) * 99 / 255 / 100);
}
