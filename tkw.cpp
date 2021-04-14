#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <thread>
#include <filesystem>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <ncurses.h>

#include "Field.hpp"
#include "Utils.hpp"

std::vector<std::string> errors;

int black;
int white;

void drawField(const vec2& _pos, const Object* _obj) {
	int bg = (_pos.x + _pos.y)%2?black:white;
	if (_obj) {
		int fg = _obj->getColor();
		const form_t& form =_obj->getForm();
		const char* itr = form;
		int start = (FORM_WIDTH - _obj->nSymbols()) / 2;
		std::size_t i = 0;
		while(i < Symbols.size() && !_obj->hasSymbol(i)){++i;}
		for(int y = 0; y < FORM_HEIGHT; ++y) {
			for(int x = 0; x < FORM_WIDTH; ++x) {
				int c = ' ';
				if (y == FORM_HEIGHT / 2 && x == FORM_WIDTH / 2) {
					switch(_obj->getSize()) {
						case Size::SMALL: c = 'S' | A_BOLD; break;
						case Size::MEDIUM: c = 'M' | A_BOLD; break;
						case Size::LARGE: c = 'L' | A_BOLD; break;
						default: c = ' ';
					}
				}
				if (y == FORM_HEIGHT - 1) {
					int p = x - start;
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
	auto end = _objs.end();
	vec2 pos = {0,0};
	for(pos.y = 0; pos.y < _dim.y; ++pos.y) {
		for (pos.x = 0; pos.x < _dim.x; ++pos.x) {
			if (itr != _objs.end() && itr->getPosition() == pos) {
				drawField(pos, &*itr);
				++itr;
				if (itr != end && itr->getPosition() == pos) {
					int count = 2;
					while(itr != end && (++itr)->getPosition() == pos) {
						++count;
					}
					errors.push_back(
							std::to_string(count) + " objs on field "
							+ std::to_string(pos.y) + " " + std::to_string(pos.x));
				}
			} else {
				drawField(pos, nullptr);
			}
		}
	}
	int y = pos.y * FORM_HEIGHT;
	for(const auto& err : errors) {
		mvprintw(++y,0,err.c_str());
		int x;
		getyx(stdscr, y, x);
	}
}

void loadAndDraw(const char* _fileName) {
	clear();
	errors.resize(0);
	std::vector<Object> objs;
	try {
		std::ifstream file(_fileName);
		json world;
		file >> world;
		for (const auto& element : world) {
			try {
				objs.emplace_back(element);
			} catch(const std::runtime_error& err) {
				errors.push_back(std::to_string(objs.size())
						+ ": " + err.what());
			}
		}
		file.close();
	} catch (const nlohmann::detail::parse_error& err) {
		errors.push_back(err.what());
	}
	std::sort(objs.begin(), objs.end());
	{
		std::bitset<Symbols.size()> check{0};
		int count = 0;
		for(const Object& obj : objs) {
			const auto& sym = obj.getSymols();
			if (auto match = sym & check; ! match.none()) {
				errors.push_back(std::to_string(count) + ": "
						+ "Symbols are not unique");
				for(std::size_t i = 0; i < Symbols.size(); ++i) {
					if(match.test(i)) {
						errors.push_back(std::string("\t:") + Symbols[i]);
					}
				}
			}
			check |= sym;
			++count;
		}
	}
	drawGrid({DIM,DIM}, objs);
	refresh();
}
int fd; ///< inotify_file descriptor
int wd; ///< inotify watcher
void drawOnChange(const char* _fileName) {
	fd = inotify_init();
	if (fd < 0) {
		perror("intotify_init");
	}
	wd = inotify_add_watch(fd, _fileName, IN_MODIFY | IN_IGNORED);
	int length;
	char buffer[BUF_LEN];
	bool run = true;
	while(run && (length = read(fd, buffer, BUF_LEN)) > 0) {
		int i = 0;
		while(i < length) {
			struct inotify_event *event = 
				reinterpret_cast<inotify_event*>(buffer + i);
			if (event->mask & IN_MODIFY) {
				loadAndDraw(_fileName);
			}
			if (event->mask & IN_IGNORED) {
				run = false;
				i = length;
			}
			i += EVENT_SIZE + event->len;
		}
	}
}

int main(int argc, const char** argv)
{
    const char* filename = fetchFilename(argc, argv);
    if (!filename) {
        return -1;
    }
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
	
	loadAndDraw(filename);
	std::thread updater(drawOnChange, filename);

	do { ch = getch(); } while(ch != 'q');
	inotify_rm_watch(fd, wd);
	updater.join();
	close(fd);

	endwin();

	return 0;
}
