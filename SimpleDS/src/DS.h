/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _DS_H_
#define _DS_H_

#include "net.h"
#include "config.h"
#include "RoboRIO.h"
#include "joystick.h"
#include "rioversions.h"
#include "narf/format.h"
#include "narf/tokenize.h"
#include "narf/bytestream.h"

#include <map>
#include <ctime>
#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <algorithm>
#include <SDL2/SDL.h>

extern Config* config;

class DS {
	private:
		uint16_t teamNum;
		uint16_t seqNum;

		Mode mode;
		Alliance alliance;
		uint8_t position;
		bool estop;
		bool enable;

		bool sentTime;
		std::future<bool> connectFuture;

		std::atomic_bool running;

		std::string firmwareVer;
		std::string libraryVer;
		std::chrono::system_clock::time_point lastVersionCheck;
		std::future<void> versionFuture;
		std::atomic_flag versionFlag;

		Net net;
		RoboRIO roborio;
		std::vector<Joystick*> joysticks;
		std::mutex jsMutex;

		std::chrono::system_clock::time_point lastSent;
		std::chrono::system_clock::time_point lastRecv;
		std::chrono::system_clock::time_point rebooting;
		std::chrono::system_clock::time_point restartingCode;

		DS(uint16_t teamNum); // : teamNum(teamNum), seqNum(1)
		void initInSocket();
		bool initOutSocket();
		void disconnect();
		void parsePacket(char* data, uint16_t size);
		std::string makePacket();
		void loadVersions();
		static DS* instance;

	public:
		static void initialize(uint16_t teamNum);
		static DS* getInstance();
		bool isConnected();
		bool hasJoysticks();
		void stop();
		void run();
		void updateJoysticks();
		void loadJoysticks();
		void saveJoysticks();
		void swapJoysticks(uint8_t a, uint8_t b);
		void reboot();
		void restartCode();
		void setAlliance(Alliance alliance);
		Alliance getAlliance();
		void setPosition(uint8_t position);
		uint8_t getPosition();
		void setEnable(bool enable = true);
		void setEStop();
		void setMode(Mode newMode);
		void toggleEnable();
		bool getEnable();
		uint16_t getTeamNum() { return teamNum; }
		std::string getLibVersion();
		std::string getFirmwareVersion();
		RoboRIO* getRoboRIO() { return &roborio; }
		std::vector<Joystick*> getJoysticks() { return joysticks; }
		std::string timePacket();
};

#endif /* _DS_H_ */
