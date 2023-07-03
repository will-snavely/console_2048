/** @file console.h
 *  @brief Functions and structures for a logical console.
 *
 *  A console stores character data in a rectangular grid.  Characters
 *  can have a color.  A console has a cursor that determines where 
 *  new characters will be placed.  When characters run past the end of 
 *  the grid, the console will scroll.
 *
 *  The console defined here should be considered seperate from the 
 *  hardware console.  I can write to a logical console without having
 *  it displayed on the screen.
 *
 *  I have taken this approach so that I can easily double buffer my console
 *  (write to a virtual console, then swap it into the "hardware" console).
 *
 *  @author Will Snavely (wsnavely)
 *  @bug None known.
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <stdint.h>

/** @brief Determines if the input is a valid color.
 */
#define IS_VALID_COLOR(C) ((C) >= 0 && (C) <= 0xFF) 

/** @brief Determines if the input is a valid character.
 */
#define IS_VALID_CHAR(C) ((C) >= 0 && (C) <= 0xFF) 

/** @brief Indicates a visible cursor.
 */
#define VISIBLE 1

/** @brief Indicates an invisible cursor.
 */
#define INVISIBLE 0

#define CONSOLE_HEIGHT 25
#define CONSOLE_WIDTH 80
#define FGND_BLACK 0x0
#define FGND_BLUE  0x1
#define FGND_GREEN 0x2
#define FGND_CYAN  0x3
#define FGND_RED   0x4
#define FGND_MAG   0x5
#define FGND_BRWN  0x6
#define FGND_LGRAY 0x7 /* Light gray. */
#define FGND_DGRAY 0x8 /* Dark gray. */
#define FGND_BBLUE 0x9 /* Bright blue. */
#define FGND_BGRN  0xA /* Bright green. */
#define FGND_BCYAN 0xB /* Bright cyan. */
#define FGND_PINK  0xC
#define FGND_BMAG  0xD /* Bright magenta. */
#define FGND_YLLW  0xE
#define FGND_WHITE 0xF
#define BGND_BLACK 0x00
#define BGND_BLUE  0x10
#define BGND_GREEN 0x20
#define BGND_CYAN  0x30
#define BGND_RED   0x40
#define BGND_MAG   0x50
#define BGND_BRWN  0x60
#define BGND_LGRAY 0x70 /* Light gray. */

/** @brief Stores data for maintaining a cursor.
 */
typedef struct cursor_t {
    /** Row location of the cursor */
    uint32_t row;
    /** Column location of the cursor */
    uint32_t col; 
    /** Is the cursor visible? */
    uint8_t visibility; 
} cursor_t;

/** @brief Stores data for maintaining a console.
 */
typedef struct console_t {
    /** The cursor associated with this console */
    cursor_t cursor;    
    /** The address where chars are written */
    void *base_addr;    
    /** The width of the console */ 
    size_t width;      
    /** The height of the console */ 
    size_t height;      
    /** The color of empty space */
    int clear_color;    
    /** The color of characters */
    int term_color;     
} console_t;

/** @brief Store a byte into a logical console.
 * 
 * See description of putbyte in p1kern.h.  This function is identical,
 * except that it operates on a logical console.
 *
 * @param console The console to write to.
 * @param ch The byte to write.
 * @return The input character
 */
int console_putbyte(console_t *console, char ch);

/** @brief Store bytes into a logical console.
 * 
 * See description of putbytes in p1kern.h.  This function is identical,
 * except that it operates on a logical console.
 *
 * @param console The console to write to.
 * @param s A pointer to the bytes to write
 * @param len How many bytes to write.
 * @return None.
 */
void console_putbytes(console_t *console, const char *s, int len);

/** @brief Store a string into a logical console.
 * 
 * We interpret the argument as your standard c-string.
 *
 * @param console The console to write to.
 * @param s A pointer to the bytes to write
 * @return None.
 */
void console_putstr(console_t *console, const char *s);

/** @brief Set the cursor on a logical console.
 * 
 * See description of set_cursor in p1kern.h.  This function is identical,
 * except that it operates on a logical console.
 *
 * @param console The console to modify.
 * @param row The new cursor row
 * @param col The new cursor column
 * @return 0 on success, -1 if arguments are invalid.
 */
int console_set_cursor(console_t *console, int row, int col);

/** @brief Clears a logical console.
 * 
 * See description of clear_console in p1kern.h.  This function is identical,
 * except that it operates on a logical console.
 *
 * @param console The console clear
 * @return None.
 */
void console_clear(console_t *console);

/** @brief Draw a character on a logical console.
 * 
 * See description of draw_char in p1kern.h.  This function is identical,
 * except that it operates on a logical console.
 *
 * @param console The console to write to.
 * @param row The row of the character.
 * @param col The column of the character.
 * @param ch The character to draw.
 * @param color The color of the character.
 * @return None
 */
void console_draw_char(console_t *console, int row, int col, int ch, int color);

/** @brief Reads a byte from a logical console.
 * 
 * See description of get_char in p1kern.h.  This function is identical,
 * except that it operates on a logical console.
 *
 * @param console The console to read from.
 * @param row The row of the character.
 * @param col The column of the character.
 * @return The character at the specified index.
 */
char console_get_char(console_t* console, int row, int col);
char console_get(console_t* console, int row, int col, char *ch, char *color);

#endif
