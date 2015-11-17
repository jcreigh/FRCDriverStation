/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include "config.h"
#include "narf/format.h"
#include "narf/tokenize.h"
#include "narf/bytestream.h"
#include <cmath>
#include <chrono>
#include <vector>
#include <SDL2/SDL.h>

class Joystick {
	private:
		int device_idx;
		std::string guid;
		SDL_Joystick* js;
		SDL_Haptic* haptic;
		SDL_HapticEffect effect;
		int effectID;
		std::vector<int8_t> axes;
		std::vector<bool> buttons;
		std::vector<int16_t> hats;
		bool outputs[32];
		uint16_t rumble[2];
		std::chrono::system_clock::time_point lastRumble;
		std::string name;
		static int16_t convertHat(uint8_t h);
	public:
		enum Rumble { LEFT, RIGHT };
		Joystick();
		Joystick(std::string guid);
		Joystick(int idx);
		~Joystick();
		void open(int idx);
		void close();
		void update();
		bool isValid() { return js != nullptr; }
		int getDeviceIdx() { return device_idx; }
		std::string getName() { return name; }
		int8_t getAxis(int idx);
		std::vector<int8_t> getAxes();
		bool getButton(int idx);
		std::vector<bool> getButtons();
		int16_t getHat(int idx);
		std::vector<int16_t> getHats();
		std::string makePacket();
		void setRumble(uint16_t val, Rumble side);
		void setRumble(uint16_t left, uint16_t right);
		uint16_t getRumble(Rumble side);
		void setOutputs(uint32_t outs);
		std::vector<bool> getOutputs();
		bool getOutput(uint8_t idx);
		std::string toString();
		std::string getGUID();
};

#endif /* _JOYSTICK_H_ */
