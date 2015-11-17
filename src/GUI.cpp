/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "GUI.h"

#define FONTSIZE 12
#define WINHEIGHT 160
#define WINWIDTH 640

const SDL_Color Colors::BLACK = {0, 0, 0};
const SDL_Color Colors::WHITE = {255, 255, 255};
const SDL_Color Colors::GREEN = {0, 170, 0};
const SDL_Color Colors::RED = {255, 0, 0};
const SDL_Color Colors::DISABLED = {140, 140, 140};

DECLARE_EMBED(DroidSansMono_ttf)

GUI::GUI() {
	cursor.x = cursor.y = 0;
	offset.x = 10;
	offset.y = 10;
	valid = false;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
		printf("Error: Failed SDL_Init: %s\n", SDL_GetError());
		return;
	}

#ifndef SDL_HAPTIC_DISABLED
		if (SDL_InitSubSystem(SDL_INIT_HAPTIC) < 0) {
			printf("Error: Failed initializing Haptic subsystem: %s\n", SDL_GetError());
			return;
		}
#endif

	if (TTF_Init() < 0) {
		printf("Error: Failed to initialize fonts: %s\n", TTF_GetError());
		return;
	}

	auto fontData = SDL_RWFromMem((void*)EMBED_DATA(DroidSansMono_ttf), (int)EMBED_SIZE(DroidSansMono_ttf));

	if ((font = TTF_OpenFontRW(fontData, 0, FONTSIZE)) == NULL) {
		printf("Error: Failed to load font: %s\n", TTF_GetError());
		return;
	}

	TTF_GlyphMetrics(font, ' ', NULL, NULL, NULL, NULL, &(charSize.x));
	charSize.y = TTF_FontHeight(font);

	win = SDL_CreateWindow("SimpleDS",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			WINWIDTH, WINHEIGHT, SDL_WINDOW_OPENGL);

	if (!win) {
		printf("Error: Failed SDL_CreateWindow: %s\n", SDL_GetError());
		return;
	}

	if ((renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED)) == NULL) {
		printf("Error: Renderer could not be created: %s\n", SDL_GetError());
		return;
	}

	SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
	valid = true;
}

GUI::~GUI() {
	SDL_DestroyWindow(win);
}

void GUI::SetTitle(std::string str) {
	std::string newTitle = "SimpleDS";
	if (str.size()) {
		newTitle += ": " + str;
	}
	SDL_SetWindowTitle(win, newTitle.c_str());
}

bool GUI::readyToDraw() {
	auto now = std::chrono::system_clock::now();
	if (now - lastDraw < std::chrono::milliseconds(100)) {
		return false;
	}
	lastDraw = now;
	return true;
}

bool GUI::isValid() {
	return valid;
}

void GUI::clear() {
	SDL_RenderClear(renderer);
}

void GUI::render() {
	SDL_RenderPresent(renderer);
}

void GUI::setTitle(std::string str) {
	std::string t = "SimpleDS";
	if (str.size() > 0) {
		t += ": " + str;
	}
	SDL_SetWindowTitle(win, t.c_str());
}

void GUI::drawChar(int x, int y, char ch, SDL_Color color /*= Colors::BLACK*/) {
	// Terribly inefficient, but who cares for this application. We don't need high fps.
	SDL_Surface* textSurface = TTF_RenderGlyph_Blended(font, ch, color);
	auto texture = SDL_CreateTextureFromSurface(renderer, textSurface);
	int minX;
	TTF_GlyphMetrics(font, ch, &minX, NULL, NULL, NULL, NULL);
	SDL_Rect renderQuad = {offset.x + charSize.x * x + minX, offset.y + charSize.y * y, textSurface->w, textSurface->h};
	if (ch == '|') { // Hack to make | show up in the right place
		renderQuad.x = renderQuad.x - minX * 4 / 3;
	}
	SDL_RenderCopy(renderer, texture, NULL, &renderQuad);
	SDL_DestroyTexture(texture);
}

void GUI::drawText(int x, int y, std::vector<std::string> texts, SDL_Color color /*= Colors::BLACK*/) {
	for (uint16_t yi = 0; yi < texts.size(); yi++) {
		moveCursor(x, y + yi);
		std::string text = texts[yi];
		for (uint16_t xi = 0; xi < text.size(); xi++) {
			drawChar(cursor.x + xi, cursor.y, text[xi], color);
		}
	}
}

void GUI::drawText(int x, int y, std::string text, SDL_Color color /*= Colors::BLACK*/) {
	std::vector<std::string> texts = narf::util::tokenize(text, '\n');
	drawText(x, y, texts, color);
}

void GUI::drawText(std::string text, SDL_Color color /*= Colors::BLACK*/) {
	drawText(cursor.x, cursor.y, text, color);
}

void GUI::moveCursor(int x, int y) {
	cursor.x = x;
	cursor.y = y;
}

void GUI::moveCursorRel(int x, int y) {
	cursor.x += x;
	cursor.y += y;
}

void GUI::drawTextRel(int x, int y, std::string text, SDL_Color color /*= Colors::BLACK*/) {
	moveCursorRel(x, y);
	drawText(text, color);
}

void GUI::setOffset(int x, int y) {
	offset.x = x;
	offset.y = y;
}

int GUI::getHeight() {
	return WINHEIGHT;
}

int GUI::getWidth() {
	return WINWIDTH;
}

Point GUI::getCharSize() {
	return charSize;
}

int GUI::pollEvent(SDL_Event* event) {
	return SDL_PollEvent(event);
}

void GUI::drawScreen(Screen* scr) {
	scr->draw(this);
}
