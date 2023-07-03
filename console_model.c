/** @file console.c
 *  @brief Implementation of a logical console.
 *
 *  @author Will Snavely (wsnavely)
 *  @bug No known bugs.
 */
       
#include <string.h>
#include <stdlib.h>
#include "console_model.h"

/***** Function prototypes ******/

/** @brief Get the address of a location in the console.
 * 
 * @param console The console of interest.
 * @param row The row of the location
 * @param col The column of the location.
 * @return The address of the specified location.
 */
static void *get_addr(console_t* console, int row, int col);

/** @brief Get the address of the cursor.
 *
 * More specifically, the address of the location in the console
 * where the cursor is sitting.  It is presumed that the address
 * is valid (e.g. not NULL).
 * 
 * @param console The console of interest.
 * @return The address of the cursor.
 */
static void *get_cursor_addr(console_t *console);

/** @brief Determines if a location is within the given console.
 *
 * A location is specified as a row and column.
 *
 * @param console The console of interest.
 * @param row The row of the location
 * @param col The column of the location.
 * @return 1 if the location is valid, 0 otherwise.
 */
static int is_valid_index(console_t *console, int row, int col);

/** @brief Draws a character at a given address.
 *
 * The character can have a color.
 *
 * @param addr The address to write to.
 * @param ch The character to write.
 * @param color The color of the character.
 * @return None. 
 */
static void draw_char_at_addr(void* addr, int ch, int color);

/** @brief Scrolls the given console by one row.
 *
 * All data on the top row is lost forever.  The new row is cleared.
 *
 * @param console The console of interest.
 * @return None. 
 */
static void scroll_by_one(console_t *console);

/** @brief Advanced the cursor by one space.
 *
 * This might cause the console to scroll.  If so, 1 is 
 * returned.  Else 0 is returned.
 *
 * @param console The console of interest.
 * @return 1 if the advance causes a scroll, 0 otherwise.
 */
static int advance_cursor(console_t *console);

/** @brief Retreat the cursor by one space.
 *
 * Retreating at the beginning of a line has no effect.
 *
 * @param console The console of interest.
 * @return None.
 */
static void retreat_cursor(console_t *console);

/** @brief Move the cursor to the beginning of the next line.
 *
 * This might cause the console to scroll.  If so, 1 is 
 * returned.  Else 0 is returned.
 *
 * @param console The console of interest.
 * @return 1 if the advance causes a scroll, 0 otherwise.
 */
static int newline(console_t *console);

/** @brief Move the cursor to the beginning of the line.
 *
 * @param console The console of interest.
 * @return None.
 */
static void carriage_return(console_t *console);

/***** Function definitions ******/

void *get_addr(console_t* console, int row, int col) {
    if(console == NULL) {
        return NULL;
    }
    char* base = (char*) console->base_addr;
    return (void*)(base + 2 * (row * console->width + col));
}

void* get_cursor_addr(console_t *console) {
    if(console == NULL) {
        return NULL;
    }
    return get_addr(console, console->cursor.row, console->cursor.col);
}

int is_valid_index(console_t *console, int row, int col) {
    return  console != NULL
        &&  ((row >= 0) && (row < console->height))
        &&  ((col >= 0) && (col < console->width));
}

void draw_char_at_addr(void* addr, int ch, int color) { 
    /* 
     * First byte encodes the character, second the color. 
     * We could check that addr is non-NULL, but the idea is
     * that this function will be used internally, and all
     * callers will ensure the input is valid.
     */
    char* ch_addr = (char*) addr;
    *(ch_addr) = (char) ch;      
    *(ch_addr + 1) = (char) color;      
}

void scroll_by_one(console_t *console) {
    if(console == NULL) {
        return;
    }

    int ii;
    int h = console->height;
    int w = console->width;

    size_t save_len = 2 * h * (w - 1); /* The number of bytes to save */
    char* second_row_addr = get_addr(console, 1, 0);
    char* last_row_addr = get_addr(console, h - 1, 0);

    memmove(console->base_addr, second_row_addr, save_len);

    /* Clear the last row. */
    for(ii = 0; ii < w * 2; ii += 2) {
        draw_char_at_addr(last_row_addr + ii, ' ', console->clear_color);
    }
}

int console_putbyte(console_t *console, char ch) {
    if(console == NULL) {
        return 0;
    }

    if(ch == '\b') {
        retreat_cursor(console);
        draw_char_at_addr(get_cursor_addr(console), ' ',  console->term_color);
    } else if(ch == '\n') {
        /* newline will return 1 if we need to scroll. */
        if(newline(console)) {
            scroll_by_one(console);
        }
    } else if(ch == '\r') {
        carriage_return(console);
    } else {
        draw_char_at_addr(get_cursor_addr(console), ch,  console->term_color);

        /* advance_cursor will return 1 if we need to scroll. */
        if(advance_cursor(console)) {
            scroll_by_one(console);
        }
    }
    return ch;
}

void console_putbytes(console_t *console, const char *s, int len) {
    if(s == NULL || len < 0 || console == NULL) {
        return;
    }

    int ii;
    for(ii = 0; ii < len; ii++) {
        console_putbyte(console, *(s + ii));
    }
}

void console_putstr(console_t *console, const char *s) {
    if(s == NULL || console == NULL) {
        return;
    }

    int off = 0;
    char ch;

    while((ch = *(s + off))) {
        console_putbyte(console, ch);
        off++;
    }
}

void console_clear(console_t *console) {
    if(console == NULL) {
        return;
    }

    int ii;
    size_t iters = console->width * console->height;
    char *addr = (char*)console->base_addr;

    for(ii = 0; ii < iters; ii++) {
        draw_char_at_addr(addr, ' ', console->clear_color);
        addr += 2;
    }

    console->cursor.row = 0;
    console->cursor.col = 0;
}

void console_draw_char(
        console_t *console, 
        int row, 
        int col, 
        int ch, 
        int color) { 
    if(console == NULL || !is_valid_index(console, row, col)) {
        return;
    }
    if(!IS_VALID_CHAR(ch)) {
        return;
    }
    if(!IS_VALID_COLOR(color)) {
        return;
    }
    draw_char_at_addr(get_addr(console, row, col), ch, color);
}

char console_get_char(console_t* console, int row, int col) {
    if(console == NULL || !is_valid_index(console, row, col)) {
        return 0;
    }
    return *((char*)get_addr(console, row, col));
}

char console_get(console_t* console, int row, int col, char *ch, char *color) {
    if(console == NULL || !is_valid_index(console, row, col)) {
        return 0;
    }
    char *addr = (char*)get_addr(console, row, col);
    *ch = *addr;
    *color = *(addr + 1);
}

int console_set_cursor(console_t *console, int row, int col) {
    if(console == NULL || !is_valid_index(console, row, col)) {
        return -1;
    }
    console->cursor.row = row;
    console->cursor.col = col;
    return 0;
}

int advance_cursor(console_t *console) {
    if(console == NULL) {
        return 0;
    }

    int scroll = 0;
    if(console->cursor.col == (console->width - 1)) {
        console->cursor.col = 0;
        if(console->cursor.row == (console->height - 1)) {
            scroll = 1;
        } else {
            console->cursor.row++;
        }
    } else {
        console->cursor.col++;
    }
    return scroll;
}

void retreat_cursor(console_t *console) {
    if(console == NULL) {
        return;
    }

    if(console->cursor.col > 0) {
        console->cursor.col--;
    }
}

int newline(console_t *console) {
    if(console == NULL) {
        return 0;
    }

    int scroll = 0;
    console->cursor.col = 0;
    if(console->cursor.row == (console->height - 1)) {
        scroll = 1;
    } else {
        console->cursor.row++;
    }
    return scroll;
}

void carriage_return(console_t *console) {
    if(console == NULL) {
        return;
    }
    console->cursor.col = 0;
}
