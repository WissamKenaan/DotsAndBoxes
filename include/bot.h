/*
 * bot.h - Computer player ("hard" bot) for Dots and Boxes (Sprint 2).
 *
 * The bot only READS the board: it never changes it. It returns a move,
 * and the game loop plays that move exactly like a human move, using
 * board_validate_move() and game_play_move().
 */
#ifndef BOT_H
#define BOT_H

#include "board.h"

/*
 * Chooses a move for `bot_player` ('A' or 'B') on the current board.
 *
 * Precondition: the game is not over (at least one line is still free).
 * Guarantee:    the returned move always passes board_validate_move(),
 *               so the bot can never play an invalid or repeated line.
 */
Move bot_choose_move(const Board *board, char bot_player);

#endif /* BOT_H */
