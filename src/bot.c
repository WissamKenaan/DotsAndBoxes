/*
 * bot.c - Computer player for Dots and Boxes (Sprint 2).
 *
 * STRATEGY (rules are tried in order; the first one that applies wins):
 *
 *   Rule 1 - Capture: if a line completes a box, draw it.
 *            A line that completes two boxes at once is preferred.
 *   Rule 2 - Safe move: otherwise draw a line that does not give the
 *            opponent a box, i.e. that does not leave any box with
 *            exactly three sides drawn.
 *   Rule 3 - Smallest sacrifice: if every line gives something away,
 *            draw the line that lets the opponent take the FEWEST boxes.
 *            For each candidate line we simulate the opponent greedily
 *            taking every box it can, and count them.
 *
 * The bot never changes the real board: it tries each move on a COPY
 * of the board (Board is a small struct, so copying it is cheap).
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

/* Number of drawn sides (0-4) of the box at (box_row, box_col). */
static int count_sides(const Board *board, int box_row, int box_col)
{
    return board->horizontal[box_row][box_col]
         + board->horizontal[box_row + 1][box_col]
         + board->vertical[box_row][box_col]
         + board->vertical[box_row][box_col + 1];
}

/* Number of boxes the move would complete right now (0, 1 or 2). */
static int boxes_completed_by(const Board *board, Move move)
{
    Board copy = *board;
    return board_apply_move(&copy, move, 'X');
}

/*
 * Returns 1 if the move leaves some unclaimed box with exactly three
 * sides drawn - a box the opponent can take on the next turn.
 */
static int gives_away_box(const Board *board, Move move)
{
    Board copy = *board;
    int r;
    int c;

    board_apply_move(&copy, move, 'X');
    for (r = 0; r < BOX_ROWS; r++) {
        for (c = 0; c < BOX_COLS; c++) {
            if (copy.owner[r][c] == NO_OWNER && count_sides(&copy, r, c) == 3) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 * Simulates the opponent's reply after `move`: the opponent keeps
 * drawing lines that complete boxes for as long as it can (each box
 * gives it another turn). Returns how many boxes it would take.
 */
static int boxes_given_away(const Board *board, Move move)
{
    Board copy = *board;
    Move moves[TOTAL_LINES];
    int taken = 0;
    int found;

    board_apply_move(&copy, move, 'X');          /* the bot's sacrifice */
    do {
        int count = list_legal_moves(&copy, moves);
        int i;
        found = 0;
        for (i = 0; i < count && !found; i++) {
            if (boxes_completed_by(&copy, moves[i]) > 0) {
                taken += board_apply_move(&copy, moves[i], 'O');
                found = 1;                       /* it moves again */
            }
        }
    } while (found);
    return taken;
}

Move bot_choose_move(const Board *board, char bot_player)
{
    Move moves[TOTAL_LINES];
    int count = list_legal_moves(board, moves);
    int best_capture = -1;
    int best_capture_boxes = 0;
    int i;

    (void)bot_player; /* the rules so far do not depend on the bot's letter */

    if (count == 0) {
        Move none = {0, 0, 0, 0}; /* precondition broken: game is over */
        return none;
    }

    /* Rule 1 - Capture (prefer a double box). */
    for (i = 0; i < count; i++) {
        int boxes = boxes_completed_by(board, moves[i]);
        if (boxes > best_capture_boxes) {
            best_capture_boxes = boxes;
            best_capture = i;
        }
    }
    if (best_capture >= 0) {
        return moves[best_capture];
    }

    /* Rule 2 - Safe move. */
    for (i = 0; i < count; i++) {
        if (!gives_away_box(board, moves[i])) {
            return moves[i];
        }
    }

    /* Rule 3 - Smallest sacrifice. */
    {
        int best = 0;
        int fewest = boxes_given_away(board, moves[0]);
        for (i = 1; i < count; i++) {
            int given = boxes_given_away(board, moves[i]);
            if (given < fewest) {
                fewest = given;
                best = i;
            }
        }
        return moves[best];
    }
}
