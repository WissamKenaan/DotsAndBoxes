/*
 * display.h - Console output for Dots and Boxes.
 *
 * Every printf in the program lives in display.c, so the look of the
 * game can be changed without touching the rules or the game loop.
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "board.h"

/* Prints the board in the format shown in the project sample run. */
void display_board(const Board *board);

/* Prints the score block surrounded by lines of asterisks. */
void display_scores(int score_a, int score_b);

/* Prints the "Player X's turn..." prompt. */
void display_turn_prompt(char player);

/* Prints the reason a move was rejected. */
void display_invalid_move(const char *reason);

/* Prints the final winner or tie announcement. */
void display_result(int score_a, int score_b);

/* Printed when standard input closes before the game is finished. */
void display_input_closed(void);

#endif /* DISPLAY_H */
