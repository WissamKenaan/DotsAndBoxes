/*
 * test_board.c - Unit tests for the board, input and game modules.
 *
 * No external framework: each CHECK prints PASS/FAIL, and the program
 * exits with status 1 if anything failed (so `make test` stops).
 */
#include <stdio.h>
#include "board.h"
#include "game.h"
#include "input.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(condition, description)                         \
    do {                                                      \
        tests_run++;                                          \
        if (condition) {                                      \
            printf("  PASS  %s\n", description);              \
        } else {                                              \
            tests_failed++;                                   \
            printf("  FAIL  %s  (%s:%d)\n", description,      \
                   __FILE__, __LINE__);                       \
        }                                                     \
    } while (0)

static Move make_move(int r1, int c1, int r2, int c2)
{
    Move m;
    m.r1 = r1;
    m.c1 = c1;
    m.r2 = r2;
    m.c2 = c2;
    return m;
}

/* Draws every line on the board for `player`, counting boxes won. */
static int fill_whole_board(Board *board, char player)
{
    int r;
    int c;
    int total = 0;

    for (r = 0; r < DOT_ROWS; r++) {
        for (c = 0; c < DOT_COLS - 1; c++) {
            total += board_apply_move(board, make_move(r, c, r, c + 1), player);
        }
    }
    for (r = 0; r < DOT_ROWS - 1; r++) {
        for (c = 0; c < DOT_COLS; c++) {
            total += board_apply_move(board, make_move(r, c, r + 1, c), player);
        }
    }
    return total;
}

static void test_init(void)
{
    Board b;
    int r;
    int c;
    int all_empty = 1;

    printf("board_init\n");
    board_init(&b);
    for (r = 0; r < BOX_ROWS; r++) {
        for (c = 0; c < BOX_COLS; c++) {
            if (b.owner[r][c] != NO_OWNER) {
                all_empty = 0;
            }
        }
    }
    CHECK(all_empty, "all 20 boxes start unowned");
    CHECK(b.claimed_boxes == 0, "claimed box count starts at 0");
    CHECK(!board_is_full(&b), "new board is not full");
}

static void test_validation(void)
{
    Board b;

    printf("board_validate_move\n");
    board_init(&b);
    CHECK(board_validate_move(&b, make_move(0, 0, 0, 1)) == MOVE_OK, "horizontal neighbours are valid");
    CHECK(board_validate_move(&b, make_move(0, 0, 1, 0)) == MOVE_OK, "vertical neighbours are valid");
    CHECK(board_validate_move(&b, make_move(4, 5, 3, 5)) == MOVE_OK, "bottom-right corner, reversed order, is valid");
    CHECK(board_validate_move(&b, make_move(5, 0, 4, 0)) == MOVE_OUT_OF_RANGE, "row 5 is out of range");
    CHECK(board_validate_move(&b, make_move(0, 6, 0, 5)) == MOVE_OUT_OF_RANGE, "column 6 is out of range");
    CHECK(board_validate_move(&b, make_move(-1, 0, 0, 0)) == MOVE_OUT_OF_RANGE, "negative row is out of range");
    CHECK(board_validate_move(&b, make_move(2, 2, 2, 2)) == MOVE_SAME_DOT, "same dot twice is rejected");
    CHECK(board_validate_move(&b, make_move(0, 0, 1, 1)) == MOVE_NOT_ADJACENT, "diagonal is rejected");
    CHECK(board_validate_move(&b, make_move(0, 0, 0, 2)) == MOVE_NOT_ADJACENT, "two dots apart is rejected");

    board_apply_move(&b, make_move(0, 3, 0, 4), PLAYER_A);
    CHECK(board_validate_move(&b, make_move(0, 3, 0, 4)) == MOVE_ALREADY_DRAWN, "repeating a line is rejected");
    CHECK(board_validate_move(&b, make_move(0, 4, 0, 3)) == MOVE_ALREADY_DRAWN, "repeating a line in reverse order is rejected");
}

static void test_single_box(void)
{
    Board b;

    printf("single box completion\n");
    board_init(&b);
    CHECK(board_apply_move(&b, make_move(0, 0, 0, 1), PLAYER_A) == 0, "top side completes nothing");
    CHECK(board_apply_move(&b, make_move(1, 0, 1, 1), PLAYER_B) == 0, "bottom side completes nothing");
    CHECK(board_apply_move(&b, make_move(0, 0, 1, 0), PLAYER_A) == 0, "left side completes nothing");
    CHECK(board_apply_move(&b, make_move(1, 1, 0, 1), PLAYER_B) == 1, "right side completes the box");
    CHECK(b.owner[0][0] == PLAYER_B, "box goes to the player who drew the LAST side");
    CHECK(b.claimed_boxes == 1, "claimed count is 1");
}

/* Completes box (1,2) with each of its four sides drawn last, so every
 * branch of board_apply_move (box above/below/left/right) is exercised. */
static void test_each_side_last(void)
{
    Move sides[4];
    const char *names[4] = {
        "top side drawn last completes the box below it",
        "bottom side drawn last completes the box above it",
        "left side drawn last completes the box to its right",
        "right side drawn last completes the box to its left"
    };
    int last;
    int i;

    printf("completion from each side\n");
    sides[0] = make_move(1, 2, 1, 3); /* top    */
    sides[1] = make_move(2, 2, 2, 3); /* bottom */
    sides[2] = make_move(1, 2, 2, 2); /* left   */
    sides[3] = make_move(1, 3, 2, 3); /* right  */

    for (last = 0; last < 4; last++) {
        Board b;
        int completed = 0;
        board_init(&b);
        for (i = 0; i < 4; i++) {
            if (i != last) {
                board_apply_move(&b, sides[i], PLAYER_A);
            }
        }
        completed = board_apply_move(&b, sides[last], PLAYER_B);
        CHECK(completed == 1 && b.owner[1][2] == PLAYER_B, names[last]);
    }
}

static void test_double_box(void)
{
    Board b;

    printf("double box completion\n");
    board_init(&b);
    /* Boxes (1,1) and (1,2) share the vertical line (1,2)-(2,2).
     * Draw every other side of both boxes first. */
    board_apply_move(&b, make_move(1, 1, 1, 2), PLAYER_A); /* top of (1,1)    */
    board_apply_move(&b, make_move(2, 1, 2, 2), PLAYER_A); /* bottom of (1,1) */
    board_apply_move(&b, make_move(1, 1, 2, 1), PLAYER_A); /* left of (1,1)   */
    board_apply_move(&b, make_move(1, 2, 1, 3), PLAYER_A); /* top of (1,2)    */
    board_apply_move(&b, make_move(2, 2, 2, 3), PLAYER_A); /* bottom of (1,2) */
    board_apply_move(&b, make_move(1, 3, 2, 3), PLAYER_A); /* right of (1,2)  */
    CHECK(b.claimed_boxes == 0, "no box complete before the shared line");
    CHECK(board_apply_move(&b, make_move(2, 2, 1, 2), PLAYER_B) == 2, "shared line completes two boxes");
    CHECK(b.owner[1][1] == PLAYER_B && b.owner[1][2] == PLAYER_B, "both boxes go to Player B");
}

static void test_full_board(void)
{
    Board b;

    printf("full board\n");
    board_init(&b);
    CHECK(fill_whole_board(&b, PLAYER_A) == TOTAL_BOXES, "drawing all 49 lines claims exactly 20 boxes");
    CHECK(board_is_full(&b), "board reports full");
}

static void test_game_turns_and_scores(void)
{
    Game g;

    printf("game turns and scoring\n");
    game_init(&g);
    CHECK(g.current_player == PLAYER_A, "Player A moves first");

    game_play_move(&g, make_move(0, 0, 0, 1));
    CHECK(g.current_player == PLAYER_B, "turn passes when no box is completed");

    game_play_move(&g, make_move(1, 0, 1, 1));  /* B */
    game_play_move(&g, make_move(0, 0, 1, 0));  /* A */
    CHECK(g.current_player == PLAYER_B, "turn is B before completing move");
    game_play_move(&g, make_move(0, 1, 1, 1));  /* B completes box (0,0) */
    CHECK(g.score_b == 1 && g.score_a == 0, "Player B scores 1");
    CHECK(g.current_player == PLAYER_B, "player who completes a box moves again");
}

static void test_winner(void)
{
    Game g;

    printf("game_winner\n");
    game_init(&g);
    g.score_a = 11;
    g.score_b = 9;
    CHECK(game_winner(&g) == PLAYER_A, "11-9 -> Player A wins");
    g.score_a = 7;
    g.score_b = 13;
    CHECK(game_winner(&g) == PLAYER_B, "7-13 -> Player B wins");
    g.score_a = 10;
    g.score_b = 10;
    CHECK(game_winner(&g) == 0, "10-10 -> tie");
}

static void test_input_parsing(void)
{
    Move m = {0, 0, 0, 0};

    printf("input_parse_move\n");
    CHECK(input_parse_move("0 4 0 3\n", &m) == INPUT_OK && m.r1 == 0 && m.c1 == 4 && m.r2 == 0 && m.c2 == 3,
          "\"0 4 0 3\" parses to four numbers");
    CHECK(input_parse_move("  1   2 1 3  \n", &m) == INPUT_OK, "extra spaces are allowed");
    CHECK(input_parse_move("0\t0\t0\t1\n", &m) == INPUT_OK, "tabs separate coordinates");
    CHECK(input_parse_move("0+0 0 1\n", &m) == INPUT_BAD_FORMAT, "first two coordinates need whitespace");
    CHECK(input_parse_move("0 0+0 1\n", &m) == INPUT_BAD_FORMAT, "middle coordinates need whitespace");
    CHECK(input_parse_move("0 0 0-1\n", &m) == INPUT_BAD_FORMAT, "last two coordinates need whitespace");
    CHECK(input_parse_move("0 4 0\n", &m) == INPUT_BAD_FORMAT, "three numbers are rejected");
    CHECK(input_parse_move("0 4 0 3 9\n", &m) == INPUT_BAD_FORMAT, "five numbers are rejected");
    CHECK(input_parse_move("a b c d\n", &m) == INPUT_BAD_FORMAT, "letters are rejected");
    CHECK(input_parse_move("A0 A1\n", &m) == INPUT_BAD_FORMAT, "\"A0\" style is rejected");
    CHECK(input_parse_move("\n", &m) == INPUT_BAD_FORMAT, "empty line is rejected");
    CHECK(input_parse_move("99999999999999999999 0 0 0\n", &m) == INPUT_OK && m.r1 == -1,
          "huge number is clamped to an out-of-range value (no overflow)");
}

int main(void)
{
    test_init();
    test_validation();
    test_single_box();
    test_each_side_last();
    test_double_box();
    test_full_board();
    test_game_turns_and_scores();
    test_winner();
    test_input_parsing();

    printf("\n%d tests run, %d failed\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
