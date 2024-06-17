#include <stdio.h>
#include <ncurses.h>

static const int KEY_CTL_C = 3;
static const int QUIT = KEY_CTL_C;

int main()
{
    // delay in ms
    // set very low timer to detect esc as escape sequence

	initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    set_escdelay(10);

    printw("press any key (CTRL+c to quit)...");
    for (int ch = getch(); ch != QUIT; ch = getch()) {
        clear();
        mvprintw(0, 0, "key pressed %03d: %s", ch, keyname(ch));
        refresh();
    }

	endwin();

	return 0;
}
