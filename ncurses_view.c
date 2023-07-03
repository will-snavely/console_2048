#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include "ncurses_view.h"
#include "console.h"

void init_ncurses_view() {
    initscr();
    hide_cursor();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    cbreak();
    start_color();
    init_color(COLOR_CYAN, 0, 0, 0);
    init_pair(1, COLOR_BLACK, COLOR_YELLOW);
    init_pair(2, COLOR_BLACK, COLOR_CYAN);
    init_pair(3, COLOR_BLACK, COLOR_BLUE);
    init_pair(4, COLOR_BLACK, COLOR_GREEN);
    init_pair(5, COLOR_BLACK, COLOR_RED);
    init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
}

void close_view() {
    endwin();
}

void hide_cursor() {
    curs_set(0);
}

void show_cursor() {
    curs_set(1);
}

void clear_console() {
    clear();
}

void copy_console(console_t* other) {
    int rr, cc;
    char ch, color;
    if(other == NULL) {
        return;
    }
    for(int rr = 0; rr < other->height; rr++) {
        for(int cc = 0; cc < other->width; cc++) {
	    console_get(other, rr, cc, &ch, &color);   
	    if(color > 0) {
		attron(COLOR_PAIR(color));
	    }
            mvaddch(rr, cc, ch);  
	    if(color > 0) {
		attroff(COLOR_PAIR(color));
	    }
        }
    }
    refresh();
}

int key_input(void) {
    return getch();
}
