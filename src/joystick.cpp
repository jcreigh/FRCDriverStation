/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "joystick.h"

extern Config* config;

std::string getJSGUID(int idx) {
	std::string s;
	s.resize(33);
	SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(idx), (char*)s.data(), (int)s.size());
	s.resize(32);
	return s;
}

int getJSfromGUID(std::string guid) {
	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (guid == getJSGUID(i)) {
			return i;
		}
	}
	return -1;
}

Joystick::Joystick() : js(nullptr), device_idx(-1) {
	close();
}

Joystick::Joystick(std::string guid) : Joystick() {
	close();
	if (guid.size() > 0) {
		this->guid = guid;
		open(getJSfromGUID(guid));
	}
}

Joystick::Joystick(int idx) : js(nullptr), device_idx(idx) {
	open(idx);
}

Joystick::~Joystick() {
	close();
}

void Joystick::open(int idx) {
	close();
	device_idx = idx;
	js = SDL_JoystickOpen(idx);
	if (js) {
		guid = getJSGUID(idx);
		name = std::string(SDL_JoystickName(js));
		config->setString(std::string("Joysticks.") + guid, name);
		axes.resize(SDL_JoystickNumAxes(js));
		buttons.resize(SDL_JoystickNumButtons(js));
		hats.resize(SDL_JoystickNumHats(js));
#ifndef SDL_HAPTIC_DISABLED
		haptic = SDL_HapticOpenFromJoystick(js);
		effectID = -1;
		//printf("We have haptic, let's start it up: %p\n", haptic);
		if (haptic && (SDL_HapticQuery(haptic) & SDL_HAPTIC_LEFTRIGHT)) {
			//printf("Creating haptic effect\n");
			memset(&effect, 0, sizeof(SDL_HapticEffect));
			effect.type = SDL_HAPTIC_LEFTRIGHT;
			effectID = SDL_HapticNewEffect(haptic, &effect);
			if (effectID < 0) {
				printf("Error creating new effect: %s\n", SDL_GetError());
			}
			SDL_HapticRunEffect(haptic, effectID, SDL_HAPTIC_INFINITY);
		}
#else
		haptic = nullptr;
#endif
	} else {
		std::string jsKey = std::string("Joysticks.") + guid;
		if (config->has(jsKey)) {
			name = config->getString(jsKey);
		}
	}
}

void Joystick::close() {
#ifndef SDL_HAPTIC_DISABLED
	if (haptic) {
		SDL_HapticClose(haptic);
	}
#endif
	if (js) {
		SDL_JoystickClose(js);
	}
	name = "---";
	axes.clear();
	buttons.clear();
	hats.clear();
	memset(outputs, 0, sizeof(outputs));
	memset(rumble, 0, sizeof(rumble));
}

int16_t Joystick::convertHat(uint8_t h) {
	switch (h) {
		case SDL_HAT_UP: return 0;
		case SDL_HAT_RIGHTUP: return 45;
		case SDL_HAT_RIGHT: return 90;
		case SDL_HAT_RIGHTDOWN: return 135;
		case SDL_HAT_DOWN: return 180;
		case SDL_HAT_LEFTDOWN: return 225;
		case SDL_HAT_LEFT: return 270;
		case SDL_HAT_LEFTUP: return 315;
	}
	return -1;
	/*if (h == SDL_HAT_CENTERED) {
		return -1;
	}
	int y = (h & SDL_HAT_UP) ? 1 : ((h & SDL_HAT_DOWN) ? -1 : 0);
	int x = (h & SDL_HAT_RIGHT) ? 1 : ((h & SDL_HAT_LEFT) ? -1 : 0);
	return (int)((std::atan2(x, y) * 180 / M_PI + 360)) % 360; */
}

void Joystick::update() {
	if (js) {
		for (uint8_t i = 0; i < axes.size(); i++) {
			axes[i] = (int8_t)(SDL_JoystickGetAxis(js, i) / 256);
		}
		for (uint8_t i = 0; i < buttons.size(); i++) {
			buttons[i] = SDL_JoystickGetButton(js, i);
		}
		for (uint8_t i = 0; i < buttons.size(); i++) {
			hats[i] = convertHat(SDL_JoystickGetHat(js, i));
		}
#ifndef SDL_HAPTIC_DISABLED
		if (haptic && effectID >= 0 && (std::chrono::system_clock::now() - lastRumble < std::chrono::seconds(1))) {
			effect.leftright.large_magnitude = rumble[0];
			effect.leftright.small_magnitude = rumble[1];
			SDL_HapticUpdateEffect(haptic, effectID, &effect);
		}
#endif
	}
}

int8_t Joystick::getAxis(int idx) {
	return axes[idx];
}

std::vector<int8_t> Joystick::getAxes() {
	return axes;
}

bool Joystick::getButton(int idx) {
	return buttons[idx];
}

std::vector<bool> Joystick::getButtons() {
	return buttons;
}

int16_t Joystick::getHat(int idx) {
	return hats[idx];
}

std::vector<int16_t> Joystick::getHats() {
	return hats;
}

std::string Joystick::makePacket() {
	narf::ByteStream s;
	s.write((uint8_t)0x00);
	s.write((uint8_t)0x0c); // Size, will overwrite when done
	s.write((uint8_t)axes.size());
	for (auto a : axes) {
		s.write(a);
	}
	uint8_t numButtons = (uint8_t)buttons.size();
	s.write(numButtons);
	uint16_t b = 0;
	for (uint8_t i = 0; i < numButtons; i++) {
		b = (uint16_t)((b << 1) | getButton(numButtons - i - 1));
	}
	s.write(b, BE);
	s.write((uint8_t)hats.size());
	for (auto h : hats) {
		s.write(h);
	}
	auto v = s.str();
	v[0] = (uint8_t)(v.size() - 1);
	return v;
}

void Joystick::setRumble(uint16_t val, Rumble side) {
	rumble[side] = val;
	lastRumble = std::chrono::system_clock::now();
}

void Joystick::setRumble(uint16_t left, uint16_t right) {
	setRumble(left, Rumble::LEFT);
	setRumble(right, Rumble::RIGHT);
}

uint16_t Joystick::getRumble(Rumble side) {
	return rumble[side];
}

void Joystick::setOutputs(uint32_t outs) {
	for (int i = 0; i < 32; i++) {
		outputs[i] = (outs >> i) & 1;
	}
}

std::vector<bool> Joystick::getOutputs() {
	return std::vector<bool>(outputs, outputs + 32);
}

bool Joystick::getOutput(uint8_t idx) {
	return outputs[idx];
}

std::string Joystick::toString() {
	std::string out = name + "\n";
	out += " Axes    : ";
	for (auto v : getAxes()) {
		out += narf::util::format("%4d ", v);
	}
	out += "\n Buttons : ";
	for (auto v : getButtons()) {
		out += narf::util::format("%d", v ? 1 : 0);
	}
	out += "\n Hats    : ";
	for (auto v : getHats()) {
		out += narf::util::format("%d", v);
	}
	out += "\n Outputs : ";
	for (int i = 0; i < 32; i++) {
		out += outputs[i] ? "1" : "0";
	}
	out += narf::util::format("\n Rumble  : %5d %5d", rumble[0], rumble[1]);

	return out;
}

std::string Joystick::getGUID() {
	return guid;
}
