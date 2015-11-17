/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "screen.h"

void ScreenMain::draw(GUI* gui) {
	auto ds = DS::getInstance();
	gui->drawText(0, 0, narf::util::format("Team           : %d ", ds->getTeamNum()));
	gui->drawTextRel(0, 1, narf::util::format("Communications : %s ", ds->isConnected() ? "Yes" : "No"), ds->isConnected() ? Colors::GREEN : Colors::RED);
	auto rio = ds->getRoboRIO();
	gui->drawTextRel(0, 1, narf::util::format("Robot Code     : %s ", rio->getCode() ? "Yes" : "No"), rio->getCode() ? Colors::GREEN : Colors::RED);
	gui->drawTextRel(0, 1, narf::util::format("Joysticks      : %s ", ds->hasJoysticks() ? "Yes" : "No"), ds->hasJoysticks() ? Colors::GREEN : Colors::RED);
	gui->drawTextRel(0, 1, narf::util::format("Enabled        : %s ", rio->getEnable() ? "Yes" : "No"), rio->getEnable() ? Colors::GREEN : Colors::BLACK);
	gui->drawText(30, 0, narf::util::format("SeqNum  : %d", rio->packet.seqNum));
	gui->drawTextRel(0, 1, narf::util::format("Station : %s %d", allianceNames[ds->getAlliance()].c_str(), ds->getPosition()));
	gui->drawTextRel(0, 1, narf::util::format("Battery : %.2f", rio->getBattery()));
	gui->drawTextRel(0, 1, narf::util::format("Mode    : %s", modeNames[rio->getMode()].c_str()));
	gui->drawTextRel(0, 1, narf::util::format("E-Stop  : %s ", rio->getEStop() ? "Yes" : "No"), rio->getEStop() ? Colors::RED : Colors::BLACK);
}

void ScreenInfo::draw(GUI* gui) {
	auto ds = DS::getInstance();
	if (ds->isConnected()) {
		gui->drawText(0, 0, "Versions:");
		gui->drawTextRel(1, 1, std::string("Library  : ") + ds->getLibVersion());
		gui->drawTextRel(0, 1, std::string("Firmware : ") + ds->getFirmwareVersion());
		gui->drawTextRel(0, 2, "CPUs:");
		auto rio = ds->getRoboRIO();
		gui->drawTextRel(1, 1, narf::util::format("#0: %.2f", rio->cpus[0]));
		gui->drawTextRel(0, 1, narf::util::format("#1: %.2f", rio->cpus[1]));
		gui->drawText(19, 4, "Usage:");
		gui->drawTextRel(1, 1, narf::util::format("RAM: %d MiB", rio->usage.ram / 4096));
		gui->drawTextRel(0, 1, narf::util::format("Disk: %d MiB", rio->usage.disk / 4096));
		gui->drawText(39, 1, "CAN:");
		gui->drawTextRel(1, 1, narf::util::format("Utilization %% : %d", rio->can.util));
		gui->drawTextRel(0, 1, narf::util::format("Bus Off       : %d", rio->can.busOff));
		gui->drawTextRel(0, 1, narf::util::format("TX Full       : %d", rio->can.txFull));
		gui->drawTextRel(0, 1, narf::util::format("Receive       : %d", rio->can.receive));
		gui->drawTextRel(0, 1, narf::util::format("Transmit      : %d", rio->can.transmit));
	}
}

void ScreenJoysticks::draw(GUI* gui) {
	auto ds = DS::getInstance();
	auto joysticks = ds->getJoysticks();
	gui->drawText(0, 0, "Joysticks:");
	gui->moveCursor(1, 0);
	for (size_t i = 0; i < joysticks.size(); i++) {
		std::string s = (i == idx) ? " >" : "  ";
		s += narf::util::format("%d. ", i);
		auto js = joysticks[i];
		auto buttons = js->getButtons();
		std::string name = js->getName();
		if (std::count(buttons.begin(), buttons.end(), true) > 0) {
			s[0] = '*';
		}
		if ((int8_t)i == selected) {
			s[3] = '~';
		}
		if (name.size() > 32) {
			name = name.substr(0, 28) + " ...";
		}
		gui->drawText(0, 1 + (int)i, s);
		gui->drawTextRel((int)s.size(), 0, name, js->isValid() ? Colors::BLACK : Colors::DISABLED);
	}
	gui->moveCursor(43, 0);
	if (joysticks.size() > 0 && idx < joysticks.size()) {
		gui->drawTextRel(0, 0, joysticks[idx]->toString());
		gui->drawTextRel(0, 1, std::string("GUID: ") + joysticks[idx]->getGUID());
	}
}

void ScreenJoysticks::update(SDL_Event e) {
	if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
		auto key = e.key.keysym;
		if (key.sym == SDLK_DOWN && idx < 5) {
			idx++;
			if (selected != -1) {
				DS::getInstance()->swapJoysticks(selected, idx);
				selected = idx;
			}
		} else if (key.sym == SDLK_UP && idx > 0) {
			idx--;
			if (selected != -1) {
				DS::getInstance()->swapJoysticks(selected, idx);
				selected = idx;
			}
		} else if (key.sym == SDLK_RIGHT) {
			selected = idx;
		} else if (key.sym == SDLK_LEFT) {
			selected = -1;
		}
	}
}

void ScreenControl::draw(GUI* gui) {
	auto ds = DS::getInstance();
	if (ds->isConnected()) {
		gui->drawText(0, 0, "Control Bytes:");
		auto rio = ds->getRoboRIO();
		gui->drawTextRel(16, 0, "E-Stop                 Robot Code");
		gui->drawTextRel(0, 1, "|     Brownout          |");
		gui->drawTextRel(0, 1, "|     |                 |");
		gui->drawTextRel(0, 1, "|     |   Enabled       |");
		gui->drawTextRel(0, 1, "|     |   | Mode        |");
		gui->drawTextRel(0, 1, "|     |   | | |         |");
		uint16_t control = *((uint16_t*)&rio->packet.control);
		gui->moveCursor(14, 6);
		for (int i = 15; i >= 0; i--) {
			if (i == 7) {
				gui->moveCursorRel(4, 0);
			}
			gui->drawTextRel(2, 0, narf::util::format("%d", (control >> (i > 7 ? i - 8 : i + 8)) & 1));
		}
	}
}

void ScreenHelp::draw(GUI* gui) {
	gui->drawText(0, 0, "Help:");
	gui->drawTextRel(1, 1, "Shift-1 : TeleOp");
	gui->drawTextRel(0, 1, "Shift-2 : Test");
	gui->drawTextRel(0, 1, "Shift-3 : Auton");
	gui->drawTextRel(0, 1, "0       : E-Stop");
	gui->drawText(25, 1, "e       : Toggle Enable");
	gui->drawTextRel(0, 1, "Space   : Disable");
	gui->drawTextRel(0, 1, "r       : Restart Code");
	gui->drawTextRel(0, 1, "R       : Reboot RoboRIO");
	gui->drawText(56, 1, "`       : Toggle Color");
	gui->drawTextRel(0, 1, "Ctrl-1  : Position 1");
	gui->drawTextRel(0, 1, "Ctrl-2  : Position 2");
	gui->drawTextRel(0, 1, "Ctrl-3  : Position 3");
}


