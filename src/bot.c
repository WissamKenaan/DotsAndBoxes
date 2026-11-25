/*
 * bot.c - Computer player ("hard" bot) for Dots and Boxes (Sprint 2).
 *
 * STRATEGY (rules are tried in order; the first one that applies wins):
 *
 *   Rule 1 - Capture: if a line completes a box, draw it.
 *            A line that completes two boxes at once is preferred.
 *   Rule 4 - Keep control (double-dealing): when only the last TWO boxes
 *            of a chain are left to capture and no safe line exists any
 *            more, the bot may decline them: it draws the far end of the
 *            chain instead, so the opponent receives both boxes with a
 *            single line, but must then move again and open the next
 *            chain for the bot. The bot decides by looking ahead to the
 *            end of the game (see endgame_value) and picks whichever of
 *            "take both" and "decline both" ends with the better score.
 *   Rule 2 - Safe move: otherwise draw a line that does not give the
 *            opponent a box, i.e. that does not leave any box with
 *            exactly three sides drawn.
 *   Rule 3 - Smallest sacrifice: if every line gives something away,
 *            draw the line that lets the opponent take the FEWEST boxes.
 *            For each candidate line we simulate the opponent greedily
 *            taking every box it can, and count them. On a tie, prefer
 *            a line that does NOT let the opponent use Rule 4 against us.
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
 * Finds the capturing line that completes the most boxes.
 * Returns that number of boxes (0 if no line completes a box) and
 * stores the line in *best. *captures receives how many different
 * lines complete at least one box.
 */
static int find_best_capture(const Board *board, Move *best, int *captures)
{
    Move moves[TOTAL_LINES];
    int count = list_legal_moves(board, moves);
    int best_boxes = 0;
    int i;

    *captures = 0;
    for (i = 0; i < count; i++) {
        int boxes = boxes_completed_by(board, moves[i]);
        if (boxes > 0) {
            (*captures)++;
        }
        if (boxes > best_boxes) {
            best_boxes = boxes;
            *best = moves[i];
        }
    }
    return best_boxes;
}

/* Returns 1 if at least one free line is a safe move (Rule 2). */
static int safe_move_exists(const Board *board)
{
    Move moves[TOTAL_LINES];
    int count = list_legal_moves(board, moves);
    int i;

    for (i = 0; i < count; i++) {
        if (boxes_completed_by(board, moves[i]) == 0
                && !gives_away_box(board, moves[i])) {
            return 1;
        }
    }
    return 0;
}

/*
 * Detects the moment for Rule 4: exactly two boxes are left to capture,
 * one behind the other (box X has 3 sides; drawing X's last side gives
 * box Y its 3rd side; after Y nothing else can be captured).
 * If so, stores in *deal the far side of Y - the "double-dealing" line -
 * and returns 1. Drawing that line completes no box and leaves X and Y
 * both with 3 sides, sharing one free line: the opponent takes both
 * boxes with that one line and then has to move again.
 */
static int find_double_deal(const Board *board, Move *deal)
{
    Move first;
    Move second;
    Board after_first;
    Board after_second;
    int captures;

    if (find_best_capture(board, &first, &captures) != 1 || captures != 1) {
        return 0;               /* not exactly one single-box capture */
    }
    after_first = *board;
    board_apply_move(&after_first, first, 'X');
    if (find_best_capture(&after_first, &second, &captures) != 1 || captures != 1) {
        return 0;               /* the run is not exactly two boxes long */
    }
    after_second = after_first;
    board_apply_move(&after_second, second, 'X');
    if (find_best_capture(&after_second, &first, &captures) != 0) {
        return 0;               /* more boxes would follow */
    }
    if (boxes_completed_by(board, second) != 0) {
        return 0;               /* the far line must not complete a box */
    }
    *deal = second;
    return 1;
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

/*
 * Rule 3: the line that gives the opponent the fewest boxes.
 * Tie-break: prefer a line after which the opponent is NOT at a
 * double-dealing point (e.g. open a 2-box chain in the middle, not at
 * its end, so the opponent cannot hand the two boxes back).
 */
static Move choose_sacrifice(const Board *board)
{
    Move moves[TOTAL_LINES];
    int count = list_legal_moves(board, moves);
    int best = 0;
    int fewest = TOTAL_BOXES + 1;
    int best_lets_deal = 1;
    int i;

    for (i = 0; i < count; i++) {
        int given = boxes_given_away(board, moves[i]);
        Board after = *board;
        Move unused;
        int lets_deal;

        board_apply_move(&after, moves[i], 'X');
        lets_deal = find_double_deal(&after, &unused);
        if (given < fewest || (given == fewest && best_lets_deal && !lets_deal)) {
            fewest = given;
            best = i;
            best_lets_deal = lets_deal;
        }
    }
    return moves[best];
}

/*
 * Look-ahead used by Rule 4, valid once no safe line is left (from then
 * on, drawing lines can never create a safe line again).
 * Returns the final score difference (boxes won minus boxes lost from
 * now on) for the player whose turn it is, assuming both players follow
 * this same strategy. Only one choice branches - "take both" or
 * "decline both" at each double-dealing point - so the search stays
 * small: at most 2^k end-of-game simulations, k = number of chains.
 */
static int endgame_value(const Board *board)
{
    Board b = *board;
    Move capture;
    Move deal;
    int captures;

    if (b.claimed_boxes == TOTAL_BOXES) {
        return 0;
    }
    if (find_best_capture(&b, &capture, &captures) > 0) {
        if (find_double_deal(&b, &deal)) {
            Board take = b;
            Board decline = b;
            int gained = board_apply_move(&take, capture, 'X');
            int value_take = gained + endgame_value(&take);
            int value_decline;

            board_apply_move(&decline, deal, 'X');  /* completes nothing */
            value_decline = -endgame_value(&decline);
            return value_take >= value_decline ? value_take : value_decline;
        } else {
            int gained = board_apply_move(&b, capture, 'X');
            return gained + endgame_value(&b);     /* same player again */
        }
    }
    board_apply_move(&b, choose_sacrifice(&b), 'X'); /* completes nothing */
    return -endgame_value(&b);                       /* opponent's turn */
}

Move bot_choose_move(const Board *board, char bot_player)
{
    Move moves[TOTAL_LINES];
    int count = list_legal_moves(board, moves);
    Move capture;
    Move deal;
    int captures;
    int i;

    (void)bot_player; /* the strategy is the same for 'A' and 'B' */

    if (count == 0) {
        Move none = {0, 0, 0, 0}; /* precondition broken: game is over */
        return none;
    }

    /* Rule 1 - Capture, with Rule 4 - Keep control. */
    if (find_best_capture(board, &capture, &captures) > 0) {
        if (find_double_deal(board, &deal)) {
            Board take = *board;
            Board both_taken;
            Board decline = *board;
            int value_take;
            int value_decline;

            /* Option "take": capture X now (Y follows inside the look-ahead). */
            board_apply_move(&take, capture, 'X');
            both_taken = take;
            board_apply_move(&both_taken, deal, 'X');
            /* Rule 4 only matters once no safe line is left anywhere. */
            if (both_taken.claimed_boxes < TOTAL_BOXES && !safe_move_exists(&both_taken)) {
                value_take = 1 + endgame_value(&take);
                board_apply_move(&decline, deal, 'X');
                value_decline = -endgame_value(&decline);
                if (value_decline > value_take) {
                    return deal;
                }
            }
        }
        return capture;
    }

    /* Rule 2 - Safe move. */
    for (i = 0; i < count; i++) {
        if (!gives_away_box(board, moves[i])) {
            return moves[i];
        }
    }

    /* Rule 3 - Smallest sacrifice. */
    return choose_sacrifice(board);
}
