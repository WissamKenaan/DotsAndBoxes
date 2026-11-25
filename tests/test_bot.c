/*
 * test_bot.c - Unit tests for the computer player (Sprint 2).
 *
 * These tests do not depend on HOW the bot chooses: they check the
 * promise in bot.h that every move it returns is legal. They stay valid
 * as the strategy gets smarter in later steps.
 */
#include <stdio.h>
#include "board.h"
#include "bot.h"
#include "game.h"

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

/* Small deterministic random-number generator, so every run of the
 * tests uses exactly the same "random" boards. */
static unsigned long rng_state = 12345UL;

static int rng_next(int limit)
{
    rng_state = rng_state * 1103515245UL + 12345UL;
    return (int)((rng_state / 65536UL) % (unsigned long)limit);
}

/* Plays one random legal line for `player`. Returns boxes completed. */
static int play_random_line(Board *board, char player)
{
    for (;;) {
        Move m;
        m.r1 = rng_next(DOT_ROWS);
        m.c1 = rng_next(DOT_COLS);
        if (rng_next(2) == 0) {
            m.r2 = m.r1;
            m.c2 = m.c1 + 1;
        } else {
            m.r2 = m.r1 + 1;
            m.c2 = m.c1;
        }
        if (board_validate_move(board, m) == MOVE_OK) {
            return board_apply_move(board, m, player);
        }
    }
}

static void test_bot_plays_whole_game_legally(void)
{
    Game g;
    int moves = 0;
    int all_legal = 1;

    printf("bot vs bot: a complete game\n");
    game_init(&g);
    while (!game_is_over(&g) && moves < 100) {
        Move m = bot_choose_move(&g.board, g.current_player);
        if (board_validate_move(&g.board, m) != MOVE_OK) {
            all_legal = 0;
            break;
        }
        game_play_move(&g, m);
        moves++;
    }
    CHECK(all_legal, "every bot move is legal");
    CHECK(moves == 49, "the game ends after exactly 49 lines");
    CHECK(g.score_a + g.score_b == 20, "all 20 boxes are claimed");
}

static void test_bot_legal_on_random_boards(void)
{
    int trial;
    int all_legal = 1;

    printf("bot on 2000 random partly-filled boards\n");
    for (trial = 0; trial < 2000; trial++) {
        Board b;
        int lines = rng_next(49); /* 0..48 lines already drawn */
        int i;
        Move m;

        board_init(&b);
        for (i = 0; i < lines; i++) {
            play_random_line(&b, (i % 2 == 0) ? 'A' : 'B');
        }
        m = bot_choose_move(&b, (trial % 2 == 0) ? 'A' : 'B');
        if (board_validate_move(&b, m) != MOVE_OK) {
            all_legal = 0;
            printf("  bad move on trial %d: %d %d %d %d\n",
                   trial, m.r1, m.c1, m.r2, m.c2);
            break;
        }
    }
    CHECK(all_legal, "bot never picks an invalid or repeated line");
}

static void test_bot_last_free_line(void)
{
    Board b;
    int r;
    int c;
    Move m;

    printf("bot with only one free line left\n");
    board_init(&b);
    for (r = 0; r < DOT_ROWS; r++) {
        for (c = 0; c < DOT_COLS - 1; c++) {
            board_apply_move(&b, (Move){r, c, r, c + 1}, 'A');
        }
    }
    for (r = 0; r < DOT_ROWS - 1; r++) {
        for (c = 0; c < DOT_COLS; c++) {
            if (!(r == 3 && c == 5)) {
                board_apply_move(&b, (Move){r, c, r + 1, c}, 'A');
            }
        }
    }
    m = bot_choose_move(&b, 'B');
    CHECK(m.r1 == 3 && m.c1 == 5 && m.r2 == 4 && m.c2 == 5,
          "bot finds the only free line (3,5)-(4,5)");
}

int main(void)
{
    test_bot_plays_whole_game_legally();
    test_bot_legal_on_random_boards();
    test_bot_last_free_line();

    printf("\n%d bot tests run, %d failed\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
