/*
 * game.h - Turn management and scoring for a two-player game.
 */
#ifndef GAME_H
#define GAME_H

#include "board.h"

#define PLAYER_A 'A'
#define PLAYER_B 'B'

typedef enum {
    GAME_EXIT = 0,
    GAME_TWO_PLAYERS = 1,
    GAME_VS_BOT = 2
} GameMode;

typedef struct {
    Board board;
    int score_a;
    int score_b;
    char current_player; /* PLAYER_A or PLAYER_B */
} Game;

/* Resets the board, sets both scores to 0, and gives Player A the first turn. */
void game_init(Game *game);

/*
 * Plays one validated move for the current player:
 * draws the line, awards completed boxes, updates the score, and
 * switches turns ONLY if no box was completed (a player who completes
 * a box moves again, as shown in the project sample run).
 * Returns the number of boxes completed (0, 1 or 2).
 */
int game_play_move(Game *game, Move move);

/* Returns 1 when every box has been claimed. */
int game_is_over(const Game *game);

/* Returns PLAYER_A or PLAYER_B for the winner, or 0 for a tie. */
char game_winner(const Game *game);

/*
 * Shows the mode menu, then runs the interactive game on stdin/stdout.
 * In bot mode the human is A and the bot is B.
 * Returns 0 on a finished game or EOF at the menu, 1 on EOF during play.
 */
int game_run(void);

#endif /* GAME_H */
