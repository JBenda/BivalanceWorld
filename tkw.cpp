#include <ncurses.h>
#include <iostream>

#include "Field.hpp"

int main()
{
	int ch;
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();

	printw("Type any character to see it in bold\n");
	ch = getch();
	if (ch == KEY_F(1)) {
		printw("F1 key pressed");
	} else {
		printw("Tke pressed key is ");
		attron(A_BOLD);
		printw("%c", ch);
		attroff(A_BOLD);
	}
	refresh();
	getch();
	endwin();
	std::cout << getForm(Size::LARGE, Form::CUBE);
	return 0;
}
