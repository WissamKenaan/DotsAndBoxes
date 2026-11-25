/*
 * bot.c - Computer player for Dots and Boxes (Sprint 2).
 *
 * STEP 1 VERSION: the bot plays the first free line it finds.
 * This is only a placeholder so the menu and game loop can be built
 * and tested now. The real strategy replaces bot_choose_move() later.
 */
#include "bot.h"

/* 5 rows x 5 horizontal lines + 4 rows x 6 vertical lines = 49 lines. */
#define TOTAL_LINES (DOT_ROWS * (DOT_COLS - 1) + (DOT_ROWS - 1) * DOT_COLS)

/* Fills `moves` with every line that is still free. Returns how many. */
static int list_legal_moves(const Board *board, Move moves[])
{
    int count = 0;
    int r;
    int c;

    for (r = 0; r < DOT_ROWS; r++) {
        for (c = 0; c < DOT_COLS - 1; c++) {
            if (!board->horizontal[r][c]) {
                Move m = {r, c, r, c + 1};
                moves[count++] = m;
            }
        }
    }
    for (r = 0; r < DOT_ROWS - 1; r++) {
        for (c = 0; c < DOT_COLS; c++) {
            if (!board->vertical[r][c]) {
                Move m = {r, c, r + 1, c};
                moves[count++] = m;
            }
        }
    }
    return count;
}

Move bot_choose_move(const Board *board, char bot_player)
{
    Move moves[TOTAL_LINES];
    int count = list_legal_moves(board, moves);

    (void)bot_player; /* not needed by the placeholder strategy */

    if (count == 0) {
        Move none = {0, 0, 0, 0}; /* precondition broken: game is over */
        return none;
    }
    return moves[0];
}
