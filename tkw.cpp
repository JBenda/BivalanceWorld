#include <ncurses.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

#include "Field.hpp"

int black;
int white;

void drawField(const vec2& _pos, const Object* _obj) {
	int bg = (_pos.x + _pos.y)%2?black:white;
	if (_obj) {
		int fg = _obj->getColor();
		const form_t& form =_obj->getForm();
		const char* itr = form;
		int start = (FORM_WIDTH - _obj->nSymbols()) / 2;
		int i = 0;
		while(i < Symbols.size() && !_obj->hasSymbol(i)){++i;}
		std::cout << start << "<" << _obj->nSymbols() << ">";
		for(int y = 0; y < FORM_HEIGHT; ++y) {
			for(int x = 0; x < FORM_WIDTH; ++x) {
				char c = ' ';
				if (y == FORM_HEIGHT - 1) {
					int p = x - start;
					std::cout << p;
					if (i < Symbols.size() && p >= 0) {
						c = Symbols[i++];
						while(i < Symbols.size() && !_obj->hasSymbol(i)){++i;}
					}
				}
				int color = *itr && *itr++ == 'X' ? fg : bg;
				mvaddch(_pos.y * FORM_HEIGHT + y,
						_pos.x * FORM_WIDTH + x,
						c | COLOR_PAIR(color));
			}
		}
	} else {
		for(int x = 0; x < FORM_WIDTH; ++x) {
			for(int y = 0; y < FORM_HEIGHT; ++y) {
				mvaddch(_pos.y * FORM_HEIGHT + y,
						_pos.x * FORM_WIDTH + x,
						' ' | COLOR_PAIR(bg));
			}
		}
	}
}

template<typename Cont>
void drawGrid(const vec2& _dim, const Cont& _objs) {
	auto itr = _objs.begin();
	vec2 pos = {0,0};
	for(pos.y = 0; pos.y < _dim.y; ++pos.y) {
		for (pos.x = 0; pos.x < _dim.x; ++pos.x) {
			if (itr != _objs.end() && itr->getPosition() == pos) {
				drawField(pos, &*itr);
				++itr;
			} else {
				drawField(pos, nullptr);
			}
		}
	}
}

int main()
{
	std::vector<Object> objs;
	std::ifstream file("../KW15/1-4-Wld.wld");
	json world;
	file >> world;
	file.close();
	for (const auto& element : world) {
		objs.emplace_back(element);
	}
	std::sort(objs.begin(), objs.end());
	int ch;
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	start_color();
	ColorId = 1;
	white = ColorId;
	init_pair(ColorId++, COLOR_BLACK, COLOR_WHITE);
	black = ColorId;
	init_pair(ColorId++, COLOR_WHITE, COLOR_BLACK);
	initForm();

	drawGrid({8,8}, objs);

	refresh();
	getch();
	endwin();

	return 0;
}
