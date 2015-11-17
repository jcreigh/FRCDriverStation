/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "DS.h"
#include "GUI.h"
#include "config.h"
#include "narf/tokenize.h"
#include "narf/embed.h"
#include "narf/format.h"
#include "screen.h"
#include "version.h"
#include <string>
#include <vector>
#include <algorithm>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define DEFAULTCONFIG "./simpleds.conf"

std::vector<const char*> args;
Config* config;
bool verbose = false;

bool hasOpt(const std::string arg) {
	return std::find(args.begin(), args.end(), arg) != args.end();
}

std::string getOpt(const std::string arg) {
	auto v = std::find(args.begin(), args.end(), arg);
	if (v != args.end() && (v + 1) != args.end()) {
		return std::string(*(v + 1));
	}
	return "";
}

void printUsage() {
	printf("Usage: %s [-h] [-V] [-c config] [teamNum]\n", args[0]);
}

int main(int argc, char* argv[]) {
	printf("SimpleDS\n");
	printf("Build: " VERSION_STR " (" SYSTEM_NAME " " SYSTEM_PROCESSOR ")");
#ifdef SDL_HAPTIC_DISABLED
	printf(" - No Haptic");
#endif
	printf("\nAuthors: " VERSION_AUTHORS "\n");

	args = std::vector<const char*>(argv, argv + argc);
	int offset = 1;

	std::string configFile = "./simpleds.conf";
	if (hasOpt("-c") || hasOpt("--config")) {
		configFile = getOpt("-c");
		if (configFile.size() == 0) {
			configFile = getOpt("--config");
		}
		offset += 2;
	}
	config = new Config(configFile);
	if (config->loaded) {
		printf("Config: %s\n", configFile.c_str());
		if (config->has("DS.foo")) {
			printf("Foo: %s\n", config->getString("DS.foo").c_str());
		}
	} else {
		printf("Config file couldn't be loaded, won't be able to save\n");
	}

	uint16_t teamNum = (uint16_t)config->getInt32("DS.team");

	verbose = (hasOpt("-v") || hasOpt("--verbose"));

	if (hasOpt("-V") || hasOpt("--version")) {
		return 0;
	}

	if (hasOpt("-h") || hasOpt("--help")) {
		printUsage();
		printf("Options:\n");
		printf(" -h, --help      Prints this message\n");
		printf(" -v, --verbose   Output debugging information\n");
		printf(" -V, --version   Prints version info and exits\n");
		printf(" -c, --config    Sets the config file to use [default: ./simpleds.conf]\n");
		printf(" teamNum         The team number to use, must be provided here or in configuration file\n");
		return 0;
	}

	if (teamNum == 0 && args.begin() + offset == args.end()) { // We're out of arguments
		printUsage();
		return 1;
	} else {
		char* endptr;
		uint16_t argTeamNum = (uint16_t)std::strtol(args[offset], &endptr, 10);

		if (endptr == args[offset] || *endptr != '\0') { // No team number in config, and none provided
			if (teamNum == 0) {
				printUsage();
				return 1;
			}
		} else {
			teamNum = argTeamNum;
		}
	}

	printf("Team Number: %d\n", teamNum);

	bool quit = false;
	SDL_Event e;

	auto gui = new GUI();
	if (!gui->isValid()) {
		return 1;
	}

	config->setInt32("DS.team", teamNum);
	config->initInt32("DS.alliance", Alliance::RED);
	config->initInt32("DS.position", 1);

	DS::initialize(teamNum);
	auto ds = DS::getInstance();

	enum GUIMode { MAIN, INFO, JOYSTICKS, CONTROL, HELP, COUNT };
	GUIMode mode = GUIMode::MAIN;

	auto runner = std::async(std::launch::async, &DS::run, ds);
	std::map<GUIMode, Screen*> screens;
	screens[MAIN] = new ScreenMain();
	screens[INFO] = new ScreenInfo();
	screens[JOYSTICKS] = new ScreenJoysticks();
	screens[CONTROL] = new ScreenControl();
	screens[HELP] = new ScreenHelp();

	while (!quit) {
		while (gui->pollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = true;
			} else if (e.type == SDL_JOYDEVICEREMOVED || e.type == SDL_JOYDEVICEADDED) {
				ds->setEnable(false);
				ds->loadJoysticks();
			} else if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
				auto key = e.key.keysym;
				if (key.sym == SDLK_e) {
					ds->toggleEnable();
				} else if (key.sym == SDLK_SPACE) {
					ds->setEnable(false);
				} else if (key.sym == SDLK_0) {
					ds->setEStop();
				} else if (key.sym == SDLK_q) {
					quit = true;
				} else if (key.sym == SDLK_r) {
					if (key.mod & KMOD_SHIFT) {
						ds->reboot();
					} else {
						ds->restartCode();
					}
				} else if (key.sym == SDLK_BACKQUOTE) {
					ds->setAlliance(ds->getAlliance() == Alliance::BLUE ? Alliance::RED : Alliance::BLUE);
				} else if (key.sym >= SDLK_1 && key.sym <= SDLK_9) {
					uint8_t keyNum = (uint8_t)(1 + key.sym - SDLK_1);
					if (key.mod & KMOD_CTRL && keyNum >= 1 && keyNum <= 3) {
						ds->setPosition(keyNum);
					} else if (key.mod & KMOD_SHIFT && keyNum >= 1 && keyNum <= 3) {
						ds->setEnable(false);
						ds->setMode((Mode)(keyNum - 1));
					} else if (keyNum >= 1 && keyNum <= GUIMode::COUNT) {
						mode = (GUIMode)(keyNum - 1);
					}
				}
			}
			screens[mode]->update(e);
		}
		if (gui->readyToDraw()) {
			gui->setOffset(10, 10);
			gui->clear();
			gui->drawScreen(screens[mode]);

			gui->setOffset(10, gui->getHeight() - gui->getCharSize().y);
			gui->drawText(0, 0, "1: Main", mode == GUIMode::MAIN ? Colors::BLACK : Colors::DISABLED);
			gui->drawTextRel(9, 0, "2: Info", mode == GUIMode::INFO ? Colors::BLACK : Colors::DISABLED);
			gui->drawTextRel(9, 0, "3: Joysticks", mode == GUIMode::JOYSTICKS ? Colors::BLACK : Colors::DISABLED);
			gui->drawTextRel(14, 0, "4: Control", mode == GUIMode::CONTROL ? Colors::BLACK : Colors::DISABLED);
			gui->drawTextRel(12, 0, "5: Help", mode == GUIMode::HELP ? Colors::BLACK : Colors::DISABLED);

			gui->render();
		}

		std::string s = narf::util::format("Team %d", teamNum);
		if (ds->isConnected()) {
			auto rio = ds->getRoboRIO();
			s += " - ";
			if (rio->getEStop()) {
				s += "E-Stopped";
			} else {
				s += narf::util::format("%s %s", modeNames[rio->getMode()].c_str(), rio->getEnable() ? "Enabled" : "Disabled");
			}
			s += narf::util::format(" - %s %d", allianceNames[ds->getAlliance()].c_str(), ds->getPosition());
			if (!rio->getCode()) {
				s += " - No Code";
			}
		} else {
			s += " - No Comms";
		}
		gui->setTitle(s);

		ds->updateJoysticks();
		SDL_Delay(25);
	}

	ds->saveJoysticks();
	ds->stop();
	SDL_Quit();
	return 0;
}

