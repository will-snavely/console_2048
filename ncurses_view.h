#ifndef _NCURSES_VIEW_H_
#define _NCURSES_VIEW_H_

#include "console_model.h"

void copy_console(console_t* other);

void init_ncurses_view(void);

void close_view(void);

void hide_cursor(void);

void show_cursor(void);

void clear_console(void);

int key_input(void);

#endif
