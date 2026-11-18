/*
 * board.h - Game board data model and rules for Dots and Boxes.
 *
 * The board has DOT_ROWS x DOT_COLS dots (5 x 6), which form
 * BOX_ROWS x BOX_COLS boxes (4 x 5 = 20 boxes).
 *
 * Lines are stored in two separate arrays:
 *   horizontal[r][c] : line from dot (r, c) to dot (r, c + 1)
 *   vertical[r][c]   : line from dot (r, c) to dot (r + 1, c)
 *
 * This module knows nothing about input or output; it only stores
 * the state of the board and enforces the rules.
 */
#ifndef BOARD_H
#define BOARD_H

#define DOT_ROWS 5
#define DOT_COLS 6
#define BOX_ROWS (DOT_ROWS - 1)
#define BOX_COLS (DOT_COLS - 1)
#define TOTAL_BOXES (BOX_ROWS * BOX_COLS)

/* Character stored in owner[][] for a box nobody has claimed yet. */
#define NO_OWNER ' '

typedef struct {
    int horizontal[DOT_ROWS][DOT_COLS - 1]; /* 1 = line drawn, 0 = empty */
    int vertical[DOT_ROWS - 1][DOT_COLS];   /* 1 = line drawn, 0 = empty */
    char owner[BOX_ROWS][BOX_COLS];         /* 'A', 'B' or NO_OWNER     */
    int claimed_boxes;                      /* number of boxes claimed  */
} Board;

/* A move: the player connects dot (r1, c1) to dot (r2, c2). */
typedef struct {
    int r1;
    int c1;
    int r2;
    int c2;
} Move;

/* Result of checking whether a move is legal. */
typedef enum {
    MOVE_OK = 0,
    MOVE_OUT_OF_RANGE,   /* a coordinate is outside the 5 x 6 dot grid    */
    MOVE_SAME_DOT,       /* both dots are the same dot                    */
    MOVE_NOT_ADJACENT,   /* dots are diagonal or more than one step apart */
    MOVE_ALREADY_DRAWN   /* the line between the dots already exists      */
} MoveStatus;

/* Sets every line to "not drawn" and every box to "no owner". */
void board_init(Board *board);

/* Checks a move against all the rules without changing the board. */
MoveStatus board_validate_move(const Board *board, Move move);

/* Returns a human-readable explanation for a MoveStatus value. */
const char *board_status_message(MoveStatus status);

/*
 * Draws the line for a move that has ALREADY been validated, then
 * claims every box the line completes for `player` ('A' or 'B').
 * Returns the number of boxes completed by this move (0, 1 or 2).
 */
int board_apply_move(Board *board, Move move, char player);

/* Returns 1 if the box at (box_row, box_col) has all four sides drawn. */
int board_is_box_complete(const Board *board, int box_row, int box_col);

/* Returns 1 when all TOTAL_BOXES boxes have been claimed. */
int board_is_full(const Board *board);

#endif /* BOARD_H */
