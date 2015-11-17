/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _GUI_H_
#define _GUI_H_

#include "narf/tokenize.h"
#include "narf/embed.h"
#include "screen.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <chrono>
#include <string>
#include <vector>

class Screen;

struct Point {
	int x;
	int y;
};

struct Colors {
	static const SDL_Color BLACK;
	static const SDL_Color GREEN;
	static const SDL_Color WHITE;
	static const SDL_Color RED;
	static const SDL_Color DISABLED;
};

class GUI {
private:
	SDL_Window* win;
	TTF_Font* font;
	SDL_Renderer* renderer;
	bool valid;
	Point offset;
	Point cursor;
	Point charSize;
	std::chrono::system_clock::time_point lastDraw;

public:
	GUI();
	~GUI();
	void SetTitle(std::string str);
	bool readyToDraw();
	bool isValid();
	void clear();
	void render();
	void setTitle(std::string str);
	void drawChar(int x, int y, char ch, SDL_Color color = Colors::BLACK);
	void drawText(int x, int y, std::vector<std::string> texts, SDL_Color color = Colors::BLACK);
	void drawText(int x, int y, std::string text, SDL_Color color = Colors::BLACK);
	void drawText(std::string text, SDL_Color color = Colors::BLACK);
	void drawTextRel(int x, int y, std::string text, SDL_Color color = Colors::BLACK);
	void moveCursor(int x, int y);
	void moveCursorRel(int x, int y);
	void setOffset(int x, int y);
	int getHeight();
	int getWidth();
	Point getCharSize();
	int pollEvent(SDL_Event* event);
	void drawScreen(Screen* scr);
};

#endif /* _GUI_H_ */
