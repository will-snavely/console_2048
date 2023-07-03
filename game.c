/** @file game.c
 *  @brief A kernel with timer, keyboard, console support
 *
 *  This file contains the kernel's main() function.
 *
 *  It sets up the drivers and starts the game.
 *  @author Will Snavely (wsnavely)
 *  @bug No known bugs.
 */

/* libc includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>

#include "console.h"
#include "ncurses_view.h"
#include "game.h"

#define STEP_DELAY 10000000 // 10ms
#define ANIM_SLOW_DOWN 1

/** @brief This array stores the main 4x4 grid of numbers. 
 */
static int number_grid[GRID_SIZE][GRID_SIZE];

/** @brief This array maintains the objects that are currently moving.
 */
static animated_block_t animated_blocks[MAX_ANIMATIONS];

/** @brief This array maintains the objects that are currently moving.
 */
static int animated_background[GRID_SIZE][GRID_SIZE];

/** @brief This buffer is used by our virtual console.
 */
static char back_buffer[CONSOLE_HEIGHT * CONSOLE_WIDTH * 2];

/** @brief Maintains the current state of the game.
 *
 * E.g. we're in the title screen, we're waiting for input, etc.
 */
static int game_state = 0;

/** @brief Counts interrupt timer ticks.
 *
 * Used to seed random number generator.
 */
static unsigned long tick_count = 0;

/** @brief Counter for the in game clock;
 */
static unsigned long game_timer = 0;

/** @brief The player's current score.
 */
static unsigned int current_score = 0;

/** @brief The overall high score.
 */
static unsigned int high_score = 0;

/** @brief Determines whether to step the game engine.
 *
 * This is set to 1 by the timer handler, and to 0 after a step is done.
 */
static volatile int process_next_step = 0;

/** @brief The tile that, when reached, indicates victory.
 */
static int winning_tile;

/** @brief A virtual console.
 *
 * We "paint" to this console then swap it into the main console,
 * for smoother animations.
 */
static console_t back_console = {
    .cursor = {0, 0, INVISIBLE},
    .base_addr = back_buffer,
    .width = CONSOLE_WIDTH,
    .height = CONSOLE_HEIGHT,
    .clear_color = 0,
    .term_color = 0
};

/** @brief Game title screen.
 */
static char* title_screen =
"*******************************************************************************\n"
"*High Score:                                                                  *\n"
"*                                                _____         .----.         *\n"
"*           .-''-.                              /    /        / .--. \\        *\n"
"*         .' .-.  )                            /    /        ' '    ' '       *\n"
"*        / .'  / /                            /    /         \\ \\    / /       *\n"
"*       (_/   / /         .-''` ''-.         /    /           `.`'--.'        *\n"
"*            / /        .'          '.      /    /  __        / `'-. `.       *\n"
"*           / /        /              `    /    /  |  |      ' /    `. \\      *\n"
"*          . '        '                '  /    '   |  |     / /       \\ '     *\n"
"*         / /    _.-')|         .-.    | /    '----|  |---.| |         | |    *\n"
"*        .' '  _.'.-'' .        |  |   ./          |  |   || |         | |    *\n"
"*       /  /.-'_.'      .       '_.'  / '----------|  |---' \\ \\       / /     *\n"
"*      /    _.'          '._         .'            |  |     `.'-...-'.'       *\n"
"*     ( _.-'                '-....-'`             /____\\       `-...-'        *\n"
"*                                                                             *\n"
"*                              The Return of Gazool                           *\n"
"*                             ~*~*~*~*~*~*~*~*~*~*~*~                         *\n"
"*                               (N)ew Game                                    *\n"
"*                               (I)nstructions                                *\n"
"*                               (Q)uit                                        *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*A horsecatdog production                        Special Thanks to: Tom Cruise*\n"
"*******************************************************************************";

/** @brief Game instruction screen.
 */
static char* instruction_screen = 
"*******************************************************************************\n"
"*                                                                             *\n"
"*                             How to Play                                     *\n"
"*                             -----------                                     *\n"
"*                             W: Shift blocks up                              *\n"
"*                             A: Shift blocks left                            *\n"
"*                             S: Shift blocks right                           *\n"
"*                             D: Shift blocks down                            *\n"
"*                             P: Pause                                        *\n"
"*                             Q: Quit                                         *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*       It's the year 2048.  The Archdemon Gazool has awoken from his         *\n"
"*       long slumber, and is hurtling towards Earth inside of a giant         *\n"
"*       comet.  You are Cliff Zimble, expert custodian and rap music          *\n"
"*       enthusiast.  Inexplicably, only you have the power to save the        *\n"
"*       world from the Ice Demon.  Even less explicably, you shall do         *\n"
"*       so by sliding tiles around.  Kind of like Ender's Game, but           *\n"
"*       way less cool.                                                        *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                      (To Leave this screen , press 'Q')                     *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*******************************************************************************";

/** @brief Game difficulty screen.
 */
static char* difficulty_screen =
"*******************************************************************************\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                       Select A Difficulty Level                             *\n"
"*                            (1) 8    -- Unicellular Organism                 *\n"
"*                            (2) 16   -- Moss                                 *\n"
"*                            (3) 32   -- Mango                                *\n"
"*                            (4) 64   -- Jellyfish                            *\n"
"*                            (5) 128  -- Cockroach                            *\n"
"*                            (6) 256  -- Hamster                              *\n"
"*                            (7) 512  -- Ferret                               *\n"
"*                            (8) 1024 -- Kangaroo                             *\n"
"*                            (9) 2048 -- Human                                *\n"
"*                            (0) 4096 -- Dolphin                              *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*                                                                             *\n"
"*******************************************************************************";

/** @brief Game defeat message.
 */
static char* defeat_message =
"********************************************************************\n"
"*        YOU LOSE -- PRESS 'Q' TO RETURN TO THE MAIN SCREEN        *\n"
"*        Or go see Tom Cruise in Jack Reacher, now on Blu-Ray.     *\n"
"********************************************************************";

/** @brief Game victory message.
 */
static char* victory_message =
"********************************************************************\n"
"*        YOU WIN -- PRESS 'Q' TO RETURN TO THE MAIN SCREEN         *\n"
"*        Way to go, Ice Man.                                       *\n"
"********************************************************************";

/** @brief Game instruction screen.
 */
static char *game_background =
"#-----------#-----------#-----------#-----------#  #-----------#-----------#\n"
"|           |           |           |           |  |  SCORE    |  TOP      |\n"
"|           |           |           |           |  #-----------#-----------#\n"
"|           |           |           |           |  |           |           |\n"
"|           |           |           |           |  #-----------#-----------#\n"
"|           |           |           |           |\n"
"#-----------#-----------#-----------#-----------#     'WASD' To move tiles\n"
"|           |           |           |           |     'Q' To quit\n"
"|           |           |           |           |\n"     
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"#-----------#-----------#-----------#-----------#\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"#-----------#-----------#-----------#-----------#\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"|           |           |           |           |\n"
"#-----------#-----------#-----------#-----------#";

/***** Block moving functions *****/

/** @brief Transpose a square array, in place.
 * 
 * @param grid The array to transpose.
 * @return None.
 */
static void transpose(int grid[GRID_SIZE][GRID_SIZE]);

/** @brief Reverse the columns of a square array, in place.
 * 
 * @param grid The array to modify.
 * @return None.
 */
static void reverse_cols(int grid[GRID_SIZE][GRID_SIZE]);

/** @brief Reverse the rows of a square array, in place.
 * 
 * @param grid The array to modify.
 * @return None.
 */
static void reverse_rows(int grid[GRID_SIZE][GRID_SIZE]);

/** @brief Rotate a square array left.
 * 
 * @param grid The array to modify.
 * @return None.
 */
static void rot_left(int grid[GRID_SIZE][GRID_SIZE]);

/** @brief Rotate a square array right.
 * 
 * @param grid The array to modify.
 * @return None.
 */
static void rot_right(int grid[GRID_SIZE][GRID_SIZE]);

/** @brief Shift the blocks in the given grid left, producing game side effects.
 *
 * Blocks are combined/shifted based on the rules of 2048.
 *
 * Side effects include: updating score and generating animations.
 *
 * The animations created use grid coordinates ((0,0) to (3,3)).  These
 * must be fixed up afterwards to refer to console coordinates.  This is
 * done because the true orientation of the board isn't known when this
 * function is called, and it's easier to fix up grid coordinates than
 * console coordinates.  See shift_left, shift_right, etc. for more
 * details.
 * 
 * @param grid The array to shift.
 * @param shift_animations An array where we store resulting animations.
 * @param shift_background Stores the squares not shifted.
 * @return 1 if something moved, 0 if nothing moved.
 */
static int shift_grid_left(
        int grid[GRID_SIZE][GRID_SIZE],
        animated_block_t shift_animations[MAX_ANIMATIONS],
        int shift_background[GRID_SIZE][GRID_SIZE]);

/** @brief Shift the blocks in the number_grid array left.
 *
 * Implemented using shift_grid_left.  Afterwards, the animation
 * coordinates are converted from grid coordinates to console
 * coordinates.
 *
 * @return None.
 */
static int shift_left();

/** @brief Shift the blocks in the number_grid array right.
 *
 * First, we reverse the rows of the number_grid array.  Then we
 * use the shift_grid_left method.  We then fix up the animation 
 * coordinates, accounting for the reverse and then converting to 
 * console coords.  We then reverse the rows again.
 *
 * @return None.
 */
static int shift_right();

/** @brief Shift the blocks in the number_grid array down.
 *
 * First, we rotate the number_grid array left.  Then we
 * use the shift_grid_left method.  We then fix up the animation 
 * coordinates, accounting for the rotation and then converting to 
 * console coords.  We then rotate the board right.
 *
 * @return None.
 */
static int shift_down();

/** @brief Shift the blocks in the number_grid array down.
 *
 * First, we rotate the number_grid array right.  Then we
 * use the shift_grid_left method.  We then fix up the animation 
 * coordinates, accounting for the rotation and then converting to 
 * console coords.  We then rotate the board left.
 *
 * @return None.
 */
static int shift_up();

/** @brief Add a new block animation to the given animation list.
 *
 * We search the list of a cell that has state == ANI_BLOCK_DEAD.
 * We then write the new animation into that cell.  If the list is 
 * full, nothing happens.
 *
 * The "animation" we're talking about here is a block shifting from
 * one grid cell to another, as a result of a user move.  The animation
 * starts in one cell (row, col) and ends in another.  The moving 
 * block has a certain value in it while it's moving, and a potentially
 * different value when it reaches the destination (if it merges
 * with another block).
 * 
 * @param animation_list The list of animations.
 * @param start_row The row where the moving block starts
 * @param start_col The column where the moving block starts
 * @param end_row The row where the moving block stops
 * @param end_col The column where the moving block stops
 * @param start_val The value in the block while it's moving.
 * @param end_val The value in the block when it stops.
 * @return None.
 */
static void add_animation(
        animated_block_t animation_list[MAX_ANIMATIONS],
        int start_row, 
        int start_col,
        int end_row,
        int end_col,
        int start_val,
        int end_val);

/** @brief Update the current score.
 * 
 * If the score exceeds the high score, then the high score changes to this 
 * value as well.
 *
 * @param score The new score.
 * @return None.
 */
static void update_score(unsigned int score);

/** @brief Determines if a block in a given grid can move.
 *
 * The rules of 2048 apply here.  This doesn't determine if a block 
 * will merge, just that it can move from its current location
 * to a different location.  Useful for determining if the 
 * game is over. 
 *
 * A block can move if one of its adjacent squares is empty, or if 
 * an adjacent square has the same value.
 * 
 * @param row The block row
 * @param col The block column 
 * @param grid The array where the block lives.
 * @return 1 if the block can move, 0 otherwise.
 */
static int can_move(int row, int col, int grid[GRID_SIZE][GRID_SIZE]);

/** @brief Add a new block to the board.
 * 
 * The block will have either 2 or 4 as its value.  It will occupy one of the 
 * remaining locations, randomly.  If the board is full, nothing happens.
 * The number_grid array is modified by this method.
 *
 * @return None.
 */
static void add_random_block();

/** @brief Draw an animation frame.
 * 
 * This function draws the next animation frame into the console.  It is only 
 * called when the game state is SHIFTING_BLOCKS.  We iterate through the
 * animated_blocks array, draw all the blocks therein. 
 *
 * The frame has two layers:
 * 1. The background, consisting of the grid outline and any non-animated blocks.
 * 2. Animated objects.  Draw on top of the background. this consists of 
 *    blocks that are still moving, and blocks that were moving and reached
 *    their desitnation (idle blocks).
 *
 * @param console The console to draw to.
 * @return None
 */
static void draw_animation_frame(console_t *console);

/** @brief Move all moving blocks by one step. 
 * 
 * This function iterates through the  list of animated objects, and 
 * moves them by one step towards their destination.  If an object reaches 
 * it's destination, it is marked as idle. If all moving objects are idle, 
 * the animation is done, and we return 0.  Otherwise we return 1.
 *
 * @return 0 if nothing moved, 1 otherwise.
 */
static int step_moving_blocks();

/** @brief Draw a block to a console, at the given row and column.
 *
 * The block will contain the given value, and this value will determine its 
 * color.
 * 
 * @param console The console to draw to.
 * @param row The console row of the upper left block corner
 * @param col The console col of the upper left block corner
 * @param value The value displayed inside the block
 * @return None.
 */
static void draw_block(console_t* console, int row, int col, int value); 

/** @brief Draw a score to a console, at the given row and column.
 * @param console The console to draw to.
 * @param row The console row where the score starts
 * @param col The console col where the score starts
 * @param score The value to render.
 * @return None.
 */
static void draw_score(console_t *console, int row, int col, unsigned int score);

/** @brief Draw a given grid of blocks.
 *
 * Iterate through the grid, and use draw_block to render them blocks to the console.
 *
 * @param console The console to draw to.
 * @param grid The collection of blocks.
 * @return None.
 */
static void draw_blocks(console_t *console, int grid[GRID_SIZE][GRID_SIZE]);

/** @brief Draw the background layer of a scene.
 * 
 * Basically, this amounts to printing a large string to the console.
 *
 * @param console The console to draw to.
 * @param screen The string containing the background.
 * @return None.
 */
static void draw_background(console_t *console, char* screen);

/** @brief Draw the main game board.
 * 
 * This draws the main game interface, when nothing is being animated (we 
 * are waiting for user input).  This consists of the grid, the blocks, 
 * the score, etc.
 *
 * @param console The console to draw to.
 * @return None.
 */
static void draw_board(console_t *console);

/** @brief Determines if the given grid represents a won game.
 *
 * The grid must contain the tile specified by the win_tile argument.
 * Returns 1 if the game is won, 0 otherwise. 
 *
 * @param grid The collection of blocks.
 * @param win_tile The winning time.
 * @return 1 if the game is won, 0 otherwise.
 */
int is_game_won(int grid[GRID_SIZE][GRID_SIZE], int win_tile);

/** @brief Determines if the given grid represents a lost game.
 *
 * A game is lost is no tiles on the board can move, either by merging
 * or moving into empty spaces. Returns 1 if the game is lost, 0 
 * otherwise. 
 *
 * @param grid The collection of blocks.
 * @return 1 if the game is lost, 0 otherwise.
 */
int is_game_lost(int grid[GRID_SIZE][GRID_SIZE]);

/** @brief Step the game engine.
 *
 * The game is driven by a state machine.  Here, we inspect the current 
 * state, perform an action, and perhaps transition to a new state. 
 * See game.h for a description of the various states, and how they 
 * transition.  This method is invoked periodically, made possible by 
 * timer interrupts (see game_tick).
 *
 * @return None.
 */
static void game_step();

void draw_block(console_t* console, int row, int col, int value) {
    static char buf[MAX_INT_STR_LEN]; /* To store the value as a string */

    int old_color = console->term_color;
    
    /* Add some color, for the kids. */
    switch(value) {
        case 2: console->term_color = 1; break;  
        case 4: console->term_color = 2; break;
        case 8: console->term_color = 3; break;  
        case 16: console->term_color = 4; break;  
        case 32: console->term_color = 5; break;  
        case 64: console->term_color = 6; break;  
        default: console->term_color = 6; break;  
    }

    console_set_cursor(console, row, col);
    console_putstr(console, "           ");
    console_set_cursor(console, row+1, col);
    console_putstr(console, "           ");
    console_set_cursor(console, row+2, col);
    snprintf(buf, MAX_INT_STR_LEN, "  %4d     ", value); 
    console_putstr(console, buf);
    console_set_cursor(console, row+3, col);
    console_putstr(console, "           ");
    console_set_cursor(console, row+4, col);
    console_putstr(console, "           ");

    console->term_color = old_color;
}

void add_random_block() {
    static int* locations[GRID_SIZE * GRID_SIZE];

    int ii, jj;
    int rand_location;
    int rand_value;
    int count = 0;
    
    /* Find all empty locations and remember their address */
    for(ii = 0; ii < GRID_SIZE; ii++) {
        for(jj = 0; jj < GRID_SIZE; jj++) {
            if(number_grid[ii][jj] == 0) {
                locations[count] = &number_grid[ii][jj];
                count++;
            }
        }
    }

    if(count == 0) {
        return;
    } else {
        /* 
         * Add a block to a random location. 
         * Should still work if there is just one.
         */
        rand_value = ((rand() % 2) + 1) * 2;
        rand_location = rand() % count;
        *(locations[rand_location]) = rand_value;
    }
}

void transpose(int grid[GRID_SIZE][GRID_SIZE]) {
    int ii, jj;
    int tmp;

    /* Swap elements across the diagonal of the array. */
    for(ii = 1; ii < GRID_SIZE; ii++) {
        for(jj = 0; jj < ii; jj++) {
            tmp = grid[ii][jj];
            grid[ii][jj] = grid[jj][ii];
            grid[jj][ii] = tmp;
        }
    }
}

void reverse_cols(int grid[GRID_SIZE][GRID_SIZE]) {
    int ii, jj;
    int tmp;
    for(ii = 0; ii < GRID_SIZE / 2; ii++) {
        for(jj = 0; jj < GRID_SIZE; jj++) {
            tmp = grid[ii][jj];
            grid[ii][jj] = grid[GRID_SIZE - ii - 1][jj];
            grid[GRID_SIZE - ii - 1][jj] = tmp;
        }
    }
}

void reverse_rows(int grid[GRID_SIZE][GRID_SIZE]) {
    int ii, jj;
    int tmp;
    for(ii = 0; ii < GRID_SIZE; ii++) {
        for(jj = 0; jj < GRID_SIZE / 2; jj++) {
            tmp = grid[ii][jj];
            grid[ii][jj] = grid[ii][GRID_SIZE - jj - 1];
            grid[ii][GRID_SIZE - jj - 1] = tmp;
        }
    }
}

void rot_left(int grid[GRID_SIZE][GRID_SIZE]) {
    /* A left rotation = transpose + reverse columns */
    transpose(grid);
    reverse_cols(grid);
}

void rot_right(int grid[GRID_SIZE][GRID_SIZE]) {
    /* A right rotation = transpose + reverse rows */
    transpose(grid);
    reverse_rows(grid);
}


void draw_blocks(console_t *console, int grid[GRID_SIZE][GRID_SIZE]) {
    int ii, jj, num;
    for(ii = 0; ii < GRID_SIZE; ii++) {
        for(jj = 0; jj < GRID_SIZE; jj++) {
            num = grid[ii][jj];
            if(num > 0) {
                /* 
                 * The CONSOLE_ROW, CONSOLE_COL convert grid coordinates 
                 * to console coordinates.
                 */ 
                draw_block(console, CONSOLE_ROW(ii), CONSOLE_COL(jj), num);
            }
        }
    }
}

void add_animation(
        animated_block_t animation_list[MAX_ANIMATIONS],
        int start_row, 
        int start_col,
        int end_row,
        int end_col,
        int start_val,
        int end_val) {
    
    int idx = 0;

    /* Find a dead cell in the animation list, if one exists */
    while(idx < MAX_ANIMATIONS) {
        if(animation_list[idx].state == ANI_BLOCK_DEAD) {
            animation_list[idx].state = ANI_BLOCK_MOVING;
            animation_list[idx].cur_row = start_row;
            animation_list[idx].cur_col = start_col;
            animation_list[idx].dest_row = end_row;
            animation_list[idx].dest_col = end_col;
            animation_list[idx].moving_value = start_val;
            animation_list[idx].idle_value = end_val;
            break;
        }
        idx++;
    }
}

void update_score(unsigned int score) {
    current_score = score;
    if(current_score > high_score) {
        high_score = current_score;
    }
}

int shift_grid_left(
        int grid[GRID_SIZE][GRID_SIZE],
        animated_block_t shift_animations[MAX_ANIMATIONS],
        int shift_background[GRID_SIZE][GRID_SIZE]) {
    int row, ii, jj;
    int cur_idx, prev_idx;
    int cur_val, prev_val;
    int combine_value;
    int something_shifted = 0;

    /* Initially, assume no blocks will move */
    for(ii = 0; ii < GRID_SIZE; ii++) {
        for(jj = 0; jj < GRID_SIZE; jj++) {
            shift_background[ii][jj] = grid[ii][jj];
        }
    }

    /* 
     * For each row:
     * Maintain a cur and prev index.  cur is the index of a block 
     * we are looking to slide.  prev either 0, or the index 
     * of the nearest, non-zero block.  If the values on these two
     * blocks matches, we merge the blocks, otherwise we slide
     * cur until it hits prev.   If prev is 0, we slide cur all the
     * way left.
     */
    for(row = 0; row < GRID_SIZE; row++) {
        prev_idx = 0;
        cur_idx = 1;
        prev_val = grid[row][prev_idx];
        cur_val = grid[row][cur_idx];

        while(cur_idx < GRID_SIZE) {
            if(cur_val > 0) {
                if(prev_val > 0) {
                    if(cur_val == prev_val) {
                        /* Merge the blocks */
                        combine_value = cur_val * 2;
                        something_shifted = 1;
                        grid[row][prev_idx] = combine_value;
                        update_score(current_score + combine_value);
                        grid[row][cur_idx] = 0;

                        add_animation(
                            shift_animations, 
                            row, 
                            cur_idx, 
                            row, 
                            prev_idx, 
                            cur_val, 
                            cur_val * 2);
                        shift_background[row][cur_idx] = 0;
                    } else if(prev_idx + 1 < cur_idx) {
                        /* 
                         * The blocks don't match, so slide one over  
                         * until is kisses the other, if they are not
                         * already adjacent.
                         * */
                        something_shifted = 1;
                        grid[row][prev_idx+1] = cur_val; 
                        grid[row][cur_idx] = 0;
                        add_animation(
                            shift_animations, 
                            row, 
                            cur_idx, 
                            row, 
                            prev_idx + 1, 
                            cur_val, 
                            cur_val);
                        shift_background[row][cur_idx] = 0;
                    }
                    
                    /* Increment prev */
                    prev_idx = prev_idx + 1;
                    prev_val = grid[row][prev_idx];
                } else {
                    /* prev_idx is 0, so slide cur left all the way */
                    something_shifted = 1;
                    grid[row][prev_idx] = cur_val;
                    grid[row][cur_idx] = 0;
                    add_animation(
                        shift_animations,
                        row, 
                        cur_idx, 
                        row, 
                        prev_idx, 
                        cur_val,    
                        cur_val);
                    shift_background[row][cur_idx] = 0;
                    prev_val = cur_val;
                } 
            }

            /* Increment cur */
            cur_idx++;
            cur_val = grid[row][cur_idx];
        }
    }    
    return something_shifted;
}

int shift_left() {
    int rt, ii;
    animated_block_t *cur;
    rt = shift_grid_left(number_grid, animated_blocks, animated_background);
    if(rt) {
        /* Fixup animation coord to refer to console coordinates */
        for(ii = 0; ii < MAX_ANIMATIONS; ii++) {
            cur = &animated_blocks[ii];
            if(cur->state == ANI_BLOCK_MOVING) {
                cur->cur_row = CONSOLE_ROW(cur->cur_row);
                cur->cur_col = CONSOLE_COL(cur->cur_col);
                cur->dest_row = CONSOLE_ROW(cur->dest_row);
                cur->dest_col = CONSOLE_COL(cur->dest_col);
            }
        }
    }
    return rt;
}

int shift_right() {
    int rt, ii;
    animated_block_t *cur;
    /* Reverse rows and shifft left */
    reverse_rows(number_grid);
    rt = shift_grid_left(number_grid, animated_blocks, animated_background);
    if(rt) {
        /* 
         * Fixup animation coord to refer to console coordinates, and 
         * to account for the reversed rows
         */
        for(ii = 0; ii < MAX_ANIMATIONS; ii++) {
            cur = &animated_blocks[ii];
            if(cur->state == ANI_BLOCK_MOVING) {
                cur->cur_row = CONSOLE_ROW(cur->cur_row);
                cur->cur_col = CONSOLE_COL(GRID_SIZE - cur->cur_col - 1);
                cur->dest_row = CONSOLE_ROW(cur->dest_row);
                cur->dest_col = CONSOLE_COL(GRID_SIZE - cur->dest_col - 1);
            }
        }
        reverse_rows(animated_background);
    }
    /* Undo the reverse */
    reverse_rows(number_grid);
    return rt;
}

int shift_down() {
    int rt, ii;
    int tmp;
    animated_block_t *cur;

    /* Rotate right and shift left */
    rot_right(number_grid);
    rt = shift_grid_left(number_grid, animated_blocks, animated_background);
    if(rt) {
        /* 
         * Fixup animation coord to refer to console coordinates, and 
         * to account for the rotation.
         */
        for(ii = 0; ii < MAX_ANIMATIONS; ii++) {
            cur = &animated_blocks[ii];
            if(cur->state == ANI_BLOCK_MOVING) {
                tmp = cur->cur_row;
                cur->cur_row = CONSOLE_ROW(GRID_SIZE - cur->cur_col - 1);
                cur->cur_col = CONSOLE_COL(tmp);

                tmp = cur->dest_row;
                cur->dest_row = CONSOLE_ROW(GRID_SIZE - cur->dest_col - 1);
                cur->dest_col = CONSOLE_COL(tmp);
            }
        }
        rot_left(animated_background);
    }
    /* Undo the rotation */
    rot_left(number_grid);
    return rt;
}

int shift_up() {
    int rt, ii;
    int tmp;
    animated_block_t *cur;

    /* Rotate left and shift left */
    rot_left(number_grid);
    rt = shift_grid_left(number_grid, animated_blocks, animated_background);
    if(rt) {
        /* 
         * Fixup animation coord to refer to console coordinates, and 
         * to account for the rotation.
         */
        for(ii = 0; ii < MAX_ANIMATIONS; ii++) {
            cur = &animated_blocks[ii];
            if(cur->state == ANI_BLOCK_MOVING) {
                tmp = cur->cur_row;
                cur->cur_row = CONSOLE_ROW(cur->cur_col);
                cur->cur_col = CONSOLE_COL(GRID_SIZE - tmp - 1);

                tmp = cur->dest_row;
                cur->dest_row = CONSOLE_ROW(cur->dest_col);
                cur->dest_col = CONSOLE_COL(GRID_SIZE - tmp - 1);
            }
        }
        rot_right(animated_background);
    }
    /* Undo the rotation */
    rot_right(number_grid);
    return rt;
}

int step_moving_blocks() {
    animated_block_t *cur;
    int something_moved = 0;
    int done_moving;
    int row_delta;
    int col_delta;
    int ii;

    for(ii = 0; ii < MAX_ANIMATIONS; ii++) {
        cur = &animated_blocks[ii];
        done_moving = 1;
        if(cur->state == ANI_BLOCK_MOVING) {
            /* 
             * Step the row and/or column by ANI_STEP_SIZE,
             * unless the row/col is closer to the destination 
             * than the step size, in which case we just close 
             * the remaining distance.
             */
            row_delta = cur->cur_row - cur->dest_row;
            col_delta = cur->cur_col - cur->dest_col;
            if(row_delta < 0) {
                row_delta = -row_delta;
                done_moving = 0;
                cur->cur_row += (row_delta < ANI_STEP_SIZE)? row_delta : ANI_STEP_SIZE;
            } else if(row_delta > 0) {
                done_moving = 0;
                cur->cur_row -= (row_delta < ANI_STEP_SIZE)? row_delta : ANI_STEP_SIZE;
            }

            if(col_delta < 0) {
                col_delta = -col_delta;
                done_moving = 0;
                cur->cur_col += (col_delta < ANI_STEP_SIZE)? col_delta : ANI_STEP_SIZE;
            } else if(col_delta > 0) {
                done_moving = 0;
                cur->cur_col -= (col_delta < ANI_STEP_SIZE)? col_delta : ANI_STEP_SIZE;
            }

            /* Block is now idle */
            if(done_moving) {
                cur->state = ANI_BLOCK_IDLE;
            } else {
                something_moved = 1;
            }
        }
    }

    return something_moved;
}

void draw_score(console_t *console, int row, int col, unsigned int score) {
    static char buf[MAX_INT_STR_LEN];
    snprintf(buf, MAX_INT_STR_LEN, "%u", score);
    console_set_cursor(console, row, col);
    console_putstr(console, buf);
}

void draw_background(console_t *console, char* screen) {
    console_clear(&back_console);
    console_set_cursor(&back_console, 0, 0);
    console_putstr(&back_console, screen);
}

void draw_board(console_t* console) {
    draw_background(console, game_background);
    draw_score(console, 3, 52, current_score);
    draw_score(console, 3, 64, high_score);
    draw_blocks(console, number_grid);
}

void draw_animation_frame(console_t* console) {
    int ii;
    animated_block_t *cur;
    draw_background(&back_console, game_background);
    draw_score(&back_console, 3, 52, current_score);
    draw_score(&back_console, 3, 64, high_score);
    draw_blocks(&back_console, animated_background);

    for(ii = 0; ii < MAX_ANIMATIONS; ii++) {
        cur = &animated_blocks[ii];
        if(cur->state == ANI_BLOCK_MOVING) {
            draw_block(&back_console, cur->cur_row, cur->cur_col, cur->moving_value);   
        } else if(cur->state == ANI_BLOCK_IDLE) {
            draw_block(&back_console, cur->cur_row, cur->cur_col, cur->idle_value);   
        }
    }
}

int can_move(int row, int col, int grid[GRID_SIZE][GRID_SIZE]) {
    int value, other;

    value = grid[row][col];

    if(value == 0) {
        return 0;
    }

    /*
     * Look at the 4 adjacent squares.  If one 
     * is empty or has a matching value, we can 
     * move (though not necessarily merge)
     */  
    if((row - 1) >= 0) {
        other = grid[row - 1][col];
        if(other == 0 || other == value) {
            return 1;
        }
    }
    if((row + 1) < GRID_SIZE) {
        other = grid[row + 1][col];
        if(other == 0 || other == value) {
            return 1;
        }
    }
    if((col - 1) >= 0) {
        other = grid[row][col - 1];
        if(other == 0 || other == value) {
            return 1;
        }
    }
    if((col + 1) < GRID_SIZE) {
        other = grid[row][col + 1];
        if(other == 0 || other == value) {
            return 1;
        }
    }

    return 0;
}

int is_game_won(int grid[GRID_SIZE][GRID_SIZE], int win_tile) {
    int ii, jj;
    int value;

    /* Is the winning tile present? */
    for(ii = 0; ii < GRID_SIZE; ii++) {
        for(jj = 0; jj < GRID_SIZE; jj++) {
            value = grid[ii][jj];
            if(value == win_tile) {
                return 1;
            } 
        }
    }    

    return 0;
}

int is_game_lost(int grid[GRID_SIZE][GRID_SIZE]) {
    int ii, jj;

    /* If any tile is empty, or we can move any tile, we haven't lost yet */
    for(ii = 0; ii < GRID_SIZE; ii++) {
        for(jj = 0; jj < GRID_SIZE; jj++) {
            if(grid[ii][jj] == 0 || can_move(ii, jj, grid)) {
                return 0;
            }
        }
    }    

    return 1;
}

void game_step() {
    int ch, ii, jj;

    /*
     * The giant state machine begins.  Every screen has two important states:
     * "ENTER" and "INPUT".  "ENTER" states indicate that we are entering a 
     * screen, e.g the title screen.  They usually involve drawing some 
     * background.  We then transition to an INPUT state.  These rarely draw 
     * anything, usually just read input from the keyboard and respond
     * (often transitioning to another state, probably an ENTER state).
     */ 
    switch(game_state) {
        case ENTER_TITLE_SCREEN:
            draw_background(&back_console, title_screen);
            draw_score(&back_console, 1, 12, high_score);
            copy_console(&back_console);
            game_state = TITLE_SCREEN_INPUT;
            break;
        case TITLE_SCREEN_INPUT:
            ch = key_input(); 
            switch(ch) {
                case 'N':
                case 'n':  
                    game_state = ENTER_DIFFICULTY_SCREEN;
                    break;
                case 'I':
                case 'i':
                    game_state = ENTER_INSTRUCTION_SCREEN;
                    break;
                case 'Q':
                case 'q':  
                    close_view();
                     exit(0);
                    break;
            } 
            break;
        case ENTER_INSTRUCTION_SCREEN:
            draw_background(&back_console, instruction_screen);
            copy_console(&back_console);
            game_state = INSTRUCTION_SCREEN_INPUT;
            break;
        case INSTRUCTION_SCREEN_INPUT:
            ch = key_input(); 
            switch(ch) {
                case 'Q':
                case 'q':
                    game_state = ENTER_TITLE_SCREEN;
                    break;
            } 
            break;
        case ENTER_DIFFICULTY_SCREEN:
            draw_background(&back_console, difficulty_screen);
            copy_console(&back_console);
            game_state = DIFFICULTY_SCREEN_INPUT;
            break;
        case DIFFICULTY_SCREEN_INPUT:
            ch = key_input(); 
            switch(ch) {
                case '1': winning_tile = 8;    game_state = GAME_START; break;
                case '2': winning_tile = 16;   game_state = GAME_START; break;
                case '3': winning_tile = 32;   game_state = GAME_START; break;
                case '4': winning_tile = 64;   game_state = GAME_START; break;
                case '5': winning_tile = 128;  game_state = GAME_START; break;
                case '6': winning_tile = 256;  game_state = GAME_START; break;
                case '7': winning_tile = 512;  game_state = GAME_START; break;
                case '8': winning_tile = 1024; game_state = GAME_START; break;
                case '9': winning_tile = 2048; game_state = GAME_START; break;
                case '0': winning_tile = 4096; game_state = GAME_START; break;
            }
            break;
        case GAME_START:
            /* Set up a new game */
            game_timer = 0;
            current_score = 0;
            for(ii = 0; ii < GRID_SIZE; ii++) {
                for(jj = 0; jj < GRID_SIZE; jj++) {
                    number_grid[ii][jj] = 0;
                }
            }
            add_random_block();
            add_random_block();
            game_state = ENTER_GAME;
        case ENTER_GAME:
            game_timer++;
            draw_board(&back_console);
            copy_console(&back_console);
            game_state = GAME_INPUT;
            break;
        case GAME_INPUT:
            game_timer++;
            ch = key_input(); 
            switch(ch) {
                case KEY_UP:
                case 'W':
                case 'w':
                    if(shift_up()) {
                        game_state = SHIFTING_BLOCKS; 
                    }
                    break;
                case KEY_DOWN:
                case 'S':
                case 's':
                    if(shift_down()) {
                        game_state = SHIFTING_BLOCKS; 
                    }
                    break;
                case KEY_LEFT:
                case 'A':
                case 'a':
                    if(shift_left()) {
                        game_state = SHIFTING_BLOCKS; 
                    }
                    break;
                case KEY_RIGHT:
                case 'D':
                case 'd':
                    if(shift_right()) {
                        game_state = SHIFTING_BLOCKS; 
                    }
                    break;
                case 'Q':
                case 'q':
                    game_state = ENTER_TITLE_SCREEN;
                    break;
            }
            break;
        case SHIFTING_BLOCKS:
            game_timer++;
            if(game_timer % ANIM_SLOW_DOWN == 0) {
                draw_animation_frame(&back_console);
                copy_console(&back_console);
                if(!step_moving_blocks()) {
                    game_state = DONE_SHIFTING_BLOCKS;
                }
            }
            break;
        case DONE_SHIFTING_BLOCKS:
            /* Animation is complete */
            game_timer++;
            for(ii = 0; ii < MAX_ANIMATIONS; ii++) {
                animated_blocks[ii].state = ANI_BLOCK_DEAD;
            }

            if(is_game_won(number_grid, winning_tile)) {
                game_state = GAME_VICTORY;
            } else {
                add_random_block();
                if(is_game_lost(number_grid)) {
                    game_state = GAME_DEFEAT;
                } else {
                    game_state = ENTER_GAME;
                }
            }
            break;
        case GAME_VICTORY:
            draw_board(&back_console);
            console_set_cursor(&back_console, 10, 0);
            console_putstr(&back_console, victory_message);
            copy_console(&back_console);
            game_state = GAME_OVER_INPUT;
            break;
        case GAME_DEFEAT:
            draw_board(&back_console);
            console_set_cursor(&back_console, 10, 0);
            console_putstr(&back_console, defeat_message);
            copy_console(&back_console);
            game_state = GAME_OVER_INPUT;
            break;
        case GAME_OVER_INPUT:
            ch = key_input(); 
            switch(ch) {
                case 'Q':
                case 'q':
                    game_state = ENTER_TITLE_SCREEN;
                    break;
            } 
            break;
    }
}

/** @brief Kernel entrypoint.
 *  
 *  This is the entrypoint for the kernel.  It simply sets up the
 *  drivers and passes control off to game_run().
 *
 * @return Does not return
 */
int main(int argc, char **argv)
{
    int ii, jj;

    srand(time(NULL));
    init_ncurses_view();

    for(ii = 0; ii < MAX_ANIMATIONS; ii++) {
        animated_blocks[ii].state = ANI_BLOCK_DEAD;
    }
    
    for(ii = 0; ii < GRID_SIZE; ii++) {
        for(jj = 0; jj < GRID_SIZE; jj++) {
            animated_background[ii][jj] = 0;
        }
    }

    game_state = ENTER_TITLE_SCREEN;
    process_next_step = 1;
    const struct timespec len = {0, STEP_DELAY};
      
    while(1) {
        game_step();
        nanosleep(&len, NULL);
    }
}

