/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "DS.h"
#include "GUI.h"
#include "narf/format.h"
#include <algorithm>

class GUI;

class Screen {
	public:
		virtual void draw(GUI* gui) = 0;
		virtual void update(SDL_Event e) {};
		virtual ~Screen() {};
};

class ScreenMain : public Screen {
	public:
		void draw(GUI* gui);
};

class ScreenInfo : public Screen {
	public:
		void draw(GUI* gui);
};

class ScreenJoysticks : public Screen {
	private:
		uint8_t idx;
		int8_t selected;
	public:
		ScreenJoysticks() : idx(0), selected(-1) {}
		void draw(GUI* gui);
		void update(SDL_Event e) override;
};

class ScreenControl : public Screen {
	public:
		void draw(GUI* gui);
};

class ScreenHelp : public Screen {
	public:
		void draw(GUI* gui);
};

#endif /* _SCREEN_H_ */
