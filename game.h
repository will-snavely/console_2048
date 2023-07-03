/** @file game.h
 *  @brief Functions/structures/defines for the main 2048 game.
 *
 *  @author Will Snavely (wsnavely)
 *  @bug None known.
 */

#ifndef __GAME_H
#define __GAME_H

/** Width/height of the game grid */
#define GRID_SIZE 4
/** Number of cells in the game grid */
#define NUM_CELLS 16

/** Maps a grid row to a console row */
#define CONSOLE_ROW(R) (((R) * 6) + 1)
/** Maps a grid column to a console column */
#define CONSOLE_COL(C) (((C) * 12) + 1)

/** The animated block isn't moving or idle. */
#define ANI_BLOCK_DEAD 0x1
/** The animated block was moving, now isn't. */
#define ANI_BLOCK_IDLE 0x2
/** The animated block is moving. */
#define ANI_BLOCK_MOVING 0x3

/** The distance in console pixels an animated block travels per step */
#define ANI_STEP_SIZE 1

/** The starting game state. */
#define ENTER_TITLE_SCREEN 0x01
/** Game state for reading input on title screen. */
#define TITLE_SCREEN_INPUT 0x02

/** Game state for entering the instruction screen. */
#define ENTER_INSTRUCTION_SCREEN 0x03
/** Game state for reading input on instruction screen. */
#define INSTRUCTION_SCREEN_INPUT 0x04

/** Game state for entering the difficulty screen. */
#define ENTER_DIFFICULTY_SCREEN 0x05
/** Game state for reading input on difficulty screen. */
#define DIFFICULTY_SCREEN_INPUT 0x06

/** Game state for starting a new 2048 round. */
#define GAME_START 0x07
/** Game state for entering a new round of the game (e.g. after a move) */
#define ENTER_GAME 0x08
/** Game state for reading input during a 2048 game. */
#define GAME_INPUT 0x09
/** Game state for entering the pause screen. */
#define ENTER_PAUSE 0x0A
/** Game state for reading input on pause screen. */
#define PAUSE_INPUT 0x0F
/** Game state for animating shifting blocks */
#define SHIFTING_BLOCKS 0x0B 
/** Game state when blocks are done shifting */
#define DONE_SHIFTING_BLOCKS 0x0C
/** Game state when victory detected */
#define GAME_VICTORY 0x0D
/** Game state when defeat detected */
#define GAME_DEFEAT 0x0E
/** Game state for reading input when game is over. */
#define GAME_OVER_INPUT 0x1F

/** Max length of a string reprenting an integer */
#define MAX_INT_STR_LEN 20

/** Max length of a string reprenting a timer */
#define MAX_TIMER_STR_LEN 40

/** Max allowable animated (idle, moving) objects */
#define MAX_ANIMATIONS 16

/** @brief An animated block.
 *
 * This struct represents a block that is moving as the result of
 * a shift operation in the 2048 game.  A block is either moving
 * (ANI_BLOCK_ACTIVE), idle (ANI_BLOCK_IDLE -- was moving but now
 * is not), or dead (ANI_BLOCK_DEAD -- not moving or idle).
 */
typedef struct animated_block_t {
    /** The value of the block while it's moving */
    uint32_t moving_value; 
    /** The value of the block when it stops moving */
    uint32_t idle_value;
    /** The current row location of the block */
    uint8_t cur_row;
    /** The current col location of the block */
    uint8_t cur_col; 
    /** The destination row of the block */
    uint8_t dest_row; 
    /** The destination col of the block */
    uint8_t dest_col; 
    /** ANI_BLOCK_DEAD, ANI_BLOCK_IDLE, ANI_BLOCK_ACTIVE */
    int state; 
} animated_block_t;

#endif
