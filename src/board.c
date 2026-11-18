/*
 * board.c - Implementation of the board data model and game rules.
 */
#include "board.h"

/* Returns 1 if (row, col) is a valid dot position, 0 otherwise. */
static int is_valid_dot(int row, int col)
{
    return row >= 0 && row < DOT_ROWS && col >= 0 && col < DOT_COLS;
}

/* Returns the absolute value of an int (avoids needing <stdlib.h>). */
static int abs_int(int value)
{
    return value < 0 ? -value : value;
}

/*
 * Reorders the two dots of a move so that (r1, c1) is always the
 * top-most / left-most dot. This lets "0 4 0 3" and "0 3 0 4" be
 * treated as the same line.
 */
static Move normalize_move(Move move)
{
    Move result = move;
    if (move.r2 < move.r1 || (move.r2 == move.r1 && move.c2 < move.c1)) {
        result.r1 = move.r2;
        result.c1 = move.c2;
        result.r2 = move.r1;
        result.c2 = move.c1;
    }
    return result;
}

/* Returns 1 if the move connects two dots in the same row. */
static int is_horizontal(Move move)
{
    return move.r1 == move.r2;
}

void board_init(Board *board)
{
    int r;
    int c;

    for (r = 0; r < DOT_ROWS; r++) {
        for (c = 0; c < DOT_COLS - 1; c++) {
            board->horizontal[r][c] = 0;
        }
    }
    for (r = 0; r < DOT_ROWS - 1; r++) {
        for (c = 0; c < DOT_COLS; c++) {
            board->vertical[r][c] = 0;
        }
    }
    for (r = 0; r < BOX_ROWS; r++) {
        for (c = 0; c < BOX_COLS; c++) {
            board->owner[r][c] = NO_OWNER;
        }
    }
    board->claimed_boxes = 0;
}

MoveStatus board_validate_move(const Board *board, Move move)
{
    Move m;
    int distance;

    if (!is_valid_dot(move.r1, move.c1) || !is_valid_dot(move.r2, move.c2)) {
        return MOVE_OUT_OF_RANGE;
    }
    if (move.r1 == move.r2 && move.c1 == move.c2) {
        return MOVE_SAME_DOT;
    }

    /* Adjacent horizontally or vertically means a Manhattan distance of 1.
     * A diagonal neighbour has distance 2, so it is rejected here too. */
    distance = abs_int(move.r1 - move.r2) + abs_int(move.c1 - move.c2);
    if (distance != 1) {
        return MOVE_NOT_ADJACENT;
    }

    m = normalize_move(move);
    if (is_horizontal(m)) {
        if (board->horizontal[m.r1][m.c1]) {
            return MOVE_ALREADY_DRAWN;
        }
    } else {
        if (board->vertical[m.r1][m.c1]) {
            return MOVE_ALREADY_DRAWN;
        }
    }
    return MOVE_OK;
}

const char *board_status_message(MoveStatus status)
{
    switch (status) {
    case MOVE_OK:
        return "Valid move.";
    case MOVE_OUT_OF_RANGE:
        return "Rows must be between 0 and 4 and columns between 0 and 5.";
    case MOVE_SAME_DOT:
        return "You must choose two different dots.";
    case MOVE_NOT_ADJACENT:
        return "Dots must be next to each other horizontally or vertically.";
    case MOVE_ALREADY_DRAWN:
        return "Those two dots are already connected.";
    }
    return "Unknown error.";
}

int board_is_box_complete(const Board *board, int box_row, int box_col)
{
    return board->horizontal[box_row][box_col]         /* top    */
        && board->horizontal[box_row + 1][box_col]     /* bottom */
        && board->vertical[box_row][box_col]           /* left   */
        && board->vertical[box_row][box_col + 1];      /* right  */
}

/*
 * If the box exists, is complete, and is not yet owned, gives it to
 * `player` and returns 1. Otherwise returns 0.
 */
static int try_claim_box(Board *board, int box_row, int box_col, char player)
{
    if (box_row < 0 || box_row >= BOX_ROWS || box_col < 0 || box_col >= BOX_COLS) {
        return 0;
    }
    if (board->owner[box_row][box_col] != NO_OWNER) {
        return 0;
    }
    if (!board_is_box_complete(board, box_row, box_col)) {
        return 0;
    }
    board->owner[box_row][box_col] = player;
    board->claimed_boxes++;
    return 1;
}

int board_apply_move(Board *board, Move move, char player)
{
    Move m = normalize_move(move);
    int completed = 0;

    /* A single line touches at most two boxes, so only those two need
     * to be checked. This makes completion detection O(1). */
    if (is_horizontal(m)) {
        board->horizontal[m.r1][m.c1] = 1;
        completed += try_claim_box(board, m.r1 - 1, m.c1, player); /* box above */
        completed += try_claim_box(board, m.r1, m.c1, player);     /* box below */
    } else {
        board->vertical[m.r1][m.c1] = 1;
        completed += try_claim_box(board, m.r1, m.c1 - 1, player); /* box left  */
        completed += try_claim_box(board, m.r1, m.c1, player);     /* box right */
    }
    return completed;
}

int board_is_full(const Board *board)
{
    return board->claimed_boxes == TOTAL_BOXES;
}
