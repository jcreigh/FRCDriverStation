/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "DS.h"

extern Config* config;
extern bool verbose;

DS* DS::instance = nullptr;

DS::DS(uint16_t teamNum) : teamNum(teamNum), versionFlag(ATOMIC_FLAG_INIT) {
	seqNum = 1;
	mode = Mode::TELEOP;
	estop = false;
	enable = false;
	sentTime = false;
	net.initSocketIn();
	loadJoysticks();
	setPosition((uint8_t)config->getInt32("DS.position"));
	setAlliance((Alliance)config->getInt32("DS.alliance"));
	connectFuture = std::async(std::launch::async, &Net::initSocketOut, &net, narf::util::format("roborio-%d.local", teamNum));
}

void DS::initialize(uint16_t teamNum) {
	if (instance == nullptr) {
		instance = new DS(teamNum);
	}
}

DS* DS::getInstance() {
	return instance;
}

bool DS::isConnected() {
	return (std::chrono::system_clock::now() - lastRecv < std::chrono::milliseconds(1000));
}

bool DS::hasJoysticks() {
	for (auto js : joysticks) {
		if (js->isValid()) {
			return true;
		}
	}
	return false;
}

void DS::stop() {
	running = false;
}

void DS::run() {
	running = true;
	while (running) {
		auto now = std::chrono::system_clock::now();
		std::string data = net.recv();
		if (data.size()) {
			if (verbose) {
				printf("In : ");
				for (auto &s : data) {
					printf("%02x ", (uint8_t)s);
				}
				printf("\n");
			}
			lastRecv = now;
			roborio.parsePacket(data);
			for (uint8_t i = 0; i < joysticks.size(); i++) {
				RoboRIO::Output outputs;
				if (enable) {
					outputs = roborio.outputs[i];
				} else {
					memset(&outputs, 0, sizeof(RoboRIO::Output));
				}
				joysticks[i]->setOutputs(outputs.outputs);
				joysticks[i]->setRumble(outputs.rumbleLeft, outputs.rumbleRight);
			}
		}
		if (now - lastSent > std::chrono::milliseconds(20)) {
			lastSent = now;
			std::string outData = makePacket();
			if (verbose) {
				printf("Out: ");
				for (auto &s : outData) {
					printf("%02x ", (uint8_t)s);
				}
				printf("\n");
			}
			net.send(outData);
		}
		if ((libraryVer.size() == 0 || firmwareVer.size() == 0) && (now - lastVersionCheck > std::chrono::milliseconds(2000))) {
			if (!versionFlag.test_and_set()) {
				versionFuture = std::async(std::launch::async, &DS::loadVersions, this);
			}
		}

		if (!isConnected() || !roborio.getCode()) {
			sentTime = false;
			estop = false;
			enable = false;
		}

		SDL_JoystickUpdate();

		std::this_thread::sleep_for(std::chrono::milliseconds(15));
	}
}

std::string DS::makePacket() {
	narf::ByteStream s;
	s.write(seqNum, BE);
	s.write((uint8_t)0x01);
	s.write((uint8_t)((estop ? (1 << 7) : 0) | (enable ? (1 << 2) : 0) | mode));

	auto now = std::chrono::system_clock::now();

	s.write((uint8_t)(((rebooting > now) ? (1 << 3) : 0) | ((restartingCode > now) ? (1 << 2) : 0)));
	s.write((uint8_t)((position - 1) + (alliance == Alliance::BLUE ? 3 : 0)));

	seqNum++;

	std::string out = s.str();
	if (!sentTime) {
		sentTime = true;
		out += timePacket();
	} else {
		if (jsMutex.try_lock()) {
			for (auto& js : joysticks) {
				out += js->makePacket();
			}
			jsMutex.unlock();
		}
	}

	return out;
}

void DS::updateJoysticks() {
	if (jsMutex.try_lock()) {
		for (auto js : joysticks) {
			js->update();
		}
		jsMutex.unlock();
	}
}

void DS::loadJoysticks() {
	jsMutex.lock();
	//printf("Unloading %ld joysticks\n", joysticks.size());
	joysticks.clear();
	//joysticks.resize(6);
	printf("Loading %d joystick%s\n", SDL_NumJoysticks(), SDL_NumJoysticks() != 1 ? "s" : "");
	std::vector<int> loadedIndexes;
	for (int i = 0; i < 6; i++) {
		auto guid = config->getString(narf::util::format("DS.joystick.%d", i));
		joysticks.push_back(new Joystick(guid));
		loadedIndexes.push_back(joysticks[i]->getDeviceIdx());
	}
	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (std::count(loadedIndexes.begin(), loadedIndexes.end(), i) == 0) {
			for (int j = 0; j < 6; j++) {
				if (joysticks[j]->getGUID() == "") { // Find first empty index
					delete joysticks[j];
					joysticks[j] = new Joystick(i);
					break;
				}
			}
		}
	}
	jsMutex.unlock();
	saveJoysticks();
}

void DS::saveJoysticks() {
	jsMutex.lock();
	for (int i = 0; i < 6; i++) {
		config->setString(narf::util::format("DS.joystick.%d", i), joysticks[i]->getGUID());
	}
	config->saveFile();
	jsMutex.unlock();
}

void DS::swapJoysticks(uint8_t a, uint8_t b) {
	enable = false;
	jsMutex.lock();
	std::swap(joysticks[a], joysticks[b]);
	jsMutex.unlock();
	saveJoysticks();
}

void DS::reboot() {
	rebooting = std::chrono::system_clock::now() + std::chrono::milliseconds(500);
}

void DS::restartCode() {
	restartingCode = std::chrono::system_clock::now() + std::chrono::milliseconds(500);
}

void DS::setAlliance(Alliance alliance) {
	config->setInt32("DS.alliance", (int)alliance);
	this->alliance = alliance;
}

Alliance DS::getAlliance() {
	return alliance;
}

void DS::setPosition(uint8_t position) {
	config->setInt32("DS.position", (int)position);
	this->position = position;
}

uint8_t DS::getPosition() {
	return position;
}

void DS::setEnable(bool enable /*= true*/) {
	this->enable = enable;
}

void DS::setEStop() {
	estop = true;
}

void DS::setMode(Mode newMode) {
	mode = newMode;
}

void DS::toggleEnable() {
	this->enable = !this->enable;
}

bool DS::getEnable() {
	return enable;
}

void DS::loadVersions() {
	lastVersionCheck = std::chrono::system_clock::now();
	libraryVer = curlLibVersion(teamNum);
	firmwareVer = curlFirmwareVersion(teamNum);
	versionFlag.clear();
}

std::string DS::getLibVersion() {
	return libraryVer;
}

std::string DS::getFirmwareVersion() {
	return firmwareVer;
}

std::string DS::timePacket() {
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	auto s = std::chrono::duration_cast<std::chrono::seconds>(ms);
	std::time_t epoch = s.count();
	std::tm* t = std::gmtime(&epoch);
	uint32_t t_ms = (uint32_t)(ms.count() % 1000);

	char tzbuf[32];
	std::strftime(tzbuf, 32, "%Z", std::localtime(&epoch));
	std::string tz(tzbuf);

	narf::ByteStream bs;
	bs.write((uint8_t)11);
	bs.write((uint8_t)0x0f);
	bs.write((uint32_t)t_ms);
	bs.write((uint8_t)t->tm_sec);
	bs.write((uint8_t)t->tm_min);
	bs.write((uint8_t)t->tm_hour);
	bs.write((uint8_t)t->tm_mday);
	bs.write((uint8_t)t->tm_mon);
	bs.write((uint8_t)t->tm_year);
	bs.write((uint8_t)(tz.size() + 1));
	bs.write((uint8_t)0x10);
	bs.write(tz);
	if (verbose) {
		printf("Sending time packet. TZ: %s\n", tz.c_str());
	}
	return bs.str();
}
