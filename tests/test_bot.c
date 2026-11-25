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

/* ---------- helpers for the strategy tests (Steps 2+) ---------- */

static int sides_of(const Board *b, int r, int c)
{
    return b->horizontal[r][c] + b->horizontal[r + 1][c]
         + b->vertical[r][c] + b->vertical[r][c + 1];
}

/* 1 if some unclaimed box has exactly 3 sides (a box is "on offer"). */
static int has_box_on_offer(const Board *b)
{
    int r;
    int c;
    for (r = 0; r < BOX_ROWS; r++) {
        for (c = 0; c < BOX_COLS; c++) {
            if (b->owner[r][c] == NO_OWNER && sides_of(b, r, c) == 3) {
                return 1;
            }
        }
    }
    return 0;
}

/* Largest number of boxes any single legal move completes right now. */
static int best_capture_available(const Board *b)
{
    int best = 0;
    int r;
    int c;

    for (r = 0; r < DOT_ROWS; r++) {
        for (c = 0; c < DOT_COLS; c++) {
            Move candidates[2];
            int k;
            candidates[0] = (Move){r, c, r, c + 1};  /* horizontal */
            candidates[1] = (Move){r, c, r + 1, c};  /* vertical   */
            for (k = 0; k < 2; k++) {
                if (board_validate_move(b, candidates[k]) == MOVE_OK) {
                    Board copy = *b;
                    int boxes = board_apply_move(&copy, candidates[k], 'X');
                    if (boxes > best) {
                        best = boxes;
                    }
                }
            }
        }
    }
    return best;
}

/* 1 if at least one legal move leaves no box on offer. */
static int safe_move_exists(const Board *b)
{
    int r;
    int c;

    for (r = 0; r < DOT_ROWS; r++) {
        for (c = 0; c < DOT_COLS; c++) {
            Move candidates[2];
            int k;
            candidates[0] = (Move){r, c, r, c + 1};
            candidates[1] = (Move){r, c, r + 1, c};
            for (k = 0; k < 2; k++) {
                if (board_validate_move(b, candidates[k]) == MOVE_OK) {
                    Board copy = *b;
                    board_apply_move(&copy, candidates[k], 'X');
                    if (!has_box_on_offer(&copy)) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* Builds a random board with `lines` random lines already drawn. */
static void random_board(Board *b, int lines)
{
    int i;
    board_init(b);
    for (i = 0; i < lines; i++) {
        play_random_line(b, (i % 2 == 0) ? 'A' : 'B');
    }
}

static void test_rule1_takes_a_free_box(void)
{
    Board b;
    Board after;
    Move m;

    printf("rule 1: capture\n");
    board_init(&b);
    board_apply_move(&b, (Move){0, 0, 0, 1}, 'A');  /* top    of box (0,0) */
    board_apply_move(&b, (Move){1, 0, 1, 1}, 'A');  /* bottom of box (0,0) */
    board_apply_move(&b, (Move){0, 0, 1, 0}, 'A');  /* left   of box (0,0) */
    m = bot_choose_move(&b, 'B');
    after = b;
    CHECK(board_apply_move(&after, m, 'B') == 1,
          "bot draws the 4th side of a 3-sided box");
}

static void test_rule1_prefers_double_box(void)
{
    Board b;
    Board after;
    Move m;

    printf("rule 1: double box preferred\n");
    board_init(&b);
    /* Box (0,0): 3 sides drawn -> a single capture is available. */
    board_apply_move(&b, (Move){0, 0, 0, 1}, 'A');
    board_apply_move(&b, (Move){1, 0, 1, 1}, 'A');
    board_apply_move(&b, (Move){0, 1, 1, 1}, 'A');
    /* Boxes (2,2) and (2,3): every side drawn except the shared line
     * (2,3)-(3,3) -> drawing it completes both. */
    board_apply_move(&b, (Move){2, 2, 2, 3}, 'A');
    board_apply_move(&b, (Move){3, 2, 3, 3}, 'A');
    board_apply_move(&b, (Move){2, 2, 3, 2}, 'A');
    board_apply_move(&b, (Move){2, 3, 2, 4}, 'A');
    board_apply_move(&b, (Move){3, 3, 3, 4}, 'A');
    board_apply_move(&b, (Move){2, 4, 3, 4}, 'A');
    m = bot_choose_move(&b, 'B');
    after = b;
    CHECK(board_apply_move(&after, m, 'B') == 2,
          "bot takes two boxes with one line instead of one");
}

static void test_rule2_avoids_giving_boxes(void)
{
    Board b;
    Board after;
    Move m;

    printf("rule 2: safe move\n");
    board_init(&b);
    /* Box (0,0) has 2 sides: drawing a 3rd side would offer it. */
    board_apply_move(&b, (Move){0, 0, 0, 1}, 'A');
    board_apply_move(&b, (Move){0, 0, 1, 0}, 'A');
    m = bot_choose_move(&b, 'B');
    after = b;
    board_apply_move(&after, m, 'B');
    CHECK(!has_box_on_offer(&after),
          "bot does not draw a 3rd side when a safe line exists");
}

static void test_rules_on_random_boards(void)
{
    int trial;
    int captures_ok = 1;
    int safe_ok = 1;
    int capture_cases = 0;
    int safe_cases = 0;

    printf("rules 1-2 on 3000 random boards\n");
    for (trial = 0; trial < 3000; trial++) {
        Board b;
        Board after;
        Move m;
        int best;

        random_board(&b, rng_next(49));
        best = best_capture_available(&b);
        m = bot_choose_move(&b, 'B');
        after = b;
        if (best > 0) {
            capture_cases++;
            if (board_apply_move(&after, m, 'B') != best) {
                captures_ok = 0;
            }
        } else if (safe_move_exists(&b)) {
            safe_cases++;
            board_apply_move(&after, m, 'B');
            if (has_box_on_offer(&after)) {
                safe_ok = 0;
            }
        }
    }
    printf("  (%d boards with a capture, %d with only safe moves)\n",
           capture_cases, safe_cases);
    CHECK(captures_ok, "whenever a capture exists, the bot takes the biggest one");
    CHECK(safe_ok, "whenever a safe move exists (and no capture), the bot plays safe");
}

/* Plays a full game: the bot is `bot_player`, the other side plays
 * random legal lines. Returns the bot's score minus the opponent's. */
static int play_bot_vs_random(char bot_player)
{
    Game g;

    game_init(&g);
    while (!game_is_over(&g)) {
        Move m;
        if (g.current_player == bot_player) {
            m = bot_choose_move(&g.board, bot_player);
        } else {
            do {
                m.r1 = rng_next(DOT_ROWS);
                m.c1 = rng_next(DOT_COLS);
                if (rng_next(2) == 0) {
                    m.r2 = m.r1;
                    m.c2 = m.c1 + 1;
                } else {
                    m.r2 = m.r1 + 1;
                    m.c2 = m.c1;
                }
            } while (board_validate_move(&g.board, m) != MOVE_OK);
        }
        game_play_move(&g, m);
    }
    return bot_player == PLAYER_A ? g.score_a - g.score_b
                                  : g.score_b - g.score_a;
}

static void test_strength_vs_random(void)
{
    int game;
    int wins = 0;
    int ties = 0;
    int losses = 0;

    printf("strength: 200 games against a random player\n");
    for (game = 0; game < 200; game++) {
        int diff = play_bot_vs_random(game % 2 == 0 ? PLAYER_B : PLAYER_A);
        if (diff > 0) {
            wins++;
        } else if (diff == 0) {
            ties++;
        } else {
            losses++;
        }
    }
    printf("  bot won %d, tied %d, lost %d\n", wins, ties, losses);
    CHECK(wins >= 190, "bot wins at least 95% of games against a random player");
}

/* ---------- Step 3: smallest sacrifice ---------- */

/* Independent count of how many boxes the opponent can take after
 * `move` if it keeps capturing greedily. */
static int opponent_greedy_gain(const Board *b, Move move)
{
    Board copy = *b;
    int taken = 0;
    int progress = 1;

    board_apply_move(&copy, move, 'X');
    while (progress) {
        int r;
        int c;
        progress = 0;
        for (r = 0; r < DOT_ROWS && !progress; r++) {
            for (c = 0; c < DOT_COLS && !progress; c++) {
                Move candidates[2];
                int k;
                candidates[0] = (Move){r, c, r, c + 1};
                candidates[1] = (Move){r, c, r + 1, c};
                for (k = 0; k < 2 && !progress; k++) {
                    if (board_validate_move(&copy, candidates[k]) == MOVE_OK) {
                        Board probe = copy;
                        if (board_apply_move(&probe, candidates[k], 'O') > 0) {
                            taken += board_apply_move(&copy, candidates[k], 'O');
                            progress = 1;
                        }
                    }
                }
            }
        }
    }
    return taken;
}

/* Draws random SAFE lines until no safe line is left: this produces the
 * typical endgame position where every move gives something away. */
static void random_endgame_board(Board *b)
{
    int tries;

    board_init(b);
    for (tries = 0; tries < 5000; tries++) {
        Move m;
        Board copy;
        if (!safe_move_exists(b)) {
            return;
        }
        m.r1 = rng_next(DOT_ROWS);
        m.c1 = rng_next(DOT_COLS);
        if (rng_next(2) == 0) {
            m.r2 = m.r1;
            m.c2 = m.c1 + 1;
        } else {
            m.r2 = m.r1 + 1;
            m.c2 = m.c1;
        }
        if (board_validate_move(b, m) != MOVE_OK) {
            continue;
        }
        copy = *b;
        board_apply_move(&copy, m, 'X');
        if (!has_box_on_offer(&copy)) {
            *b = copy;
        }
    }
}

static void test_rule3_smallest_sacrifice(void)
{
    int trial;
    int optimal = 1;
    int boards = 0;
    int r;
    int c;

    printf("rule 3: smallest sacrifice on 300 endgame boards\n");
    for (trial = 0; trial < 300; trial++) {
        Board b;
        Move m;
        int fewest = 99;
        int given;

        random_endgame_board(&b);
        if (best_capture_available(&b) > 0 || safe_move_exists(&b)) {
            continue;  /* not an endgame position */
        }
        boards++;
        for (r = 0; r < DOT_ROWS; r++) {
            for (c = 0; c < DOT_COLS; c++) {
                Move candidates[2];
                int k;
                candidates[0] = (Move){r, c, r, c + 1};
                candidates[1] = (Move){r, c, r + 1, c};
                for (k = 0; k < 2; k++) {
                    if (board_validate_move(&b, candidates[k]) == MOVE_OK) {
                        int g = opponent_greedy_gain(&b, candidates[k]);
                        if (g < fewest) {
                            fewest = g;
                        }
                    }
                }
            }
        }
        m = bot_choose_move(&b, 'B');
        given = opponent_greedy_gain(&b, m);
        if (given != fewest) {
            optimal = 0;
            printf("  trial %d: bot gives %d, minimum is %d\n", trial, given, fewest);
        }
    }
    printf("  (%d endgame boards checked)\n", boards);
    CHECK(boards >= 250, "the generator produced at least 250 endgame boards");
    CHECK(optimal, "the bot always gives away the fewest boxes possible");
}

/* The Step 2 bot, kept here as a reference opponent: capture, then a
 * safe line, otherwise the first free line. */
static Move step2_bot_move(const Board *b)
{
    Move first = {0, 0, 0, 0};
    Move safe = {0, 0, 0, 0};
    int have_first = 0;
    int have_safe = 0;
    int best_boxes = 0;
    Move best = {0, 0, 0, 0};
    int r;
    int c;

    for (r = 0; r < DOT_ROWS; r++) {
        for (c = 0; c < DOT_COLS; c++) {
            Move candidates[2];
            int k;
            candidates[0] = (Move){r, c, r, c + 1};
            candidates[1] = (Move){r, c, r + 1, c};
            for (k = 0; k < 2; k++) {
                Board copy;
                int boxes;
                if (board_validate_move(b, candidates[k]) != MOVE_OK) {
                    continue;
                }
                if (!have_first) {
                    first = candidates[k];
                    have_first = 1;
                }
                copy = *b;
                boxes = board_apply_move(&copy, candidates[k], 'X');
                if (boxes > best_boxes) {
                    best_boxes = boxes;
                    best = candidates[k];
                }
                if (!have_safe && boxes == 0 && !has_box_on_offer(&copy)) {
                    safe = candidates[k];
                    have_safe = 1;
                }
            }
        }
    }
    if (best_boxes > 0) {
        return best;
    }
    return have_safe ? safe : first;
}

/* One game between the current bot and the Step 2 bot. Both sides
 * first draw `opening` random safe lines so that every game differs.
 * Returns the current bot's score minus the Step 2 bot's score. */
static int play_current_vs_step2(char current_bot, int opening)
{
    Game g;
    int moves = 0;

    game_init(&g);
    while (!game_is_over(&g)) {
        Move m;
        if (moves < opening && safe_move_exists(&g.board)
                && best_capture_available(&g.board) == 0) {
            do {
                Board copy;
                m.r1 = rng_next(DOT_ROWS);
                m.c1 = rng_next(DOT_COLS);
                if (rng_next(2) == 0) {
                    m.r2 = m.r1;
                    m.c2 = m.c1 + 1;
                } else {
                    m.r2 = m.r1 + 1;
                    m.c2 = m.c1;
                }
                if (board_validate_move(&g.board, m) != MOVE_OK) {
                    continue;
                }
                copy = g.board;
                board_apply_move(&copy, m, 'X');
                if (!has_box_on_offer(&copy)) {
                    break;
                }
            } while (1);
        } else if (g.current_player == current_bot) {
            m = bot_choose_move(&g.board, current_bot);
        } else {
            m = step2_bot_move(&g.board);
        }
        game_play_move(&g, m);
        moves++;
    }
    return current_bot == PLAYER_A ? g.score_a - g.score_b
                                   : g.score_b - g.score_a;
}

static void test_strength_vs_step2(void)
{
    int game;
    int wins = 0;
    int ties = 0;
    int losses = 0;

    printf("strength: 400 games against the Step 2 bot\n");
    for (game = 0; game < 400; game++) {
        int opening = 8 + rng_next(15);   /* 8..22 random safe lines first */
        int diff = play_current_vs_step2(game % 2 == 0 ? PLAYER_A : PLAYER_B, opening);
        if (diff > 0) {
            wins++;
        } else if (diff == 0) {
            ties++;
        } else {
            losses++;
        }
    }
    printf("  current bot won %d, tied %d, lost %d\n", wins, ties, losses);
    CHECK(wins > losses, "the current bot beats the Step 2 bot more often than it loses");
}

int main(void)
{
    test_bot_plays_whole_game_legally();
    test_bot_legal_on_random_boards();
    test_bot_last_free_line();
    test_rule1_takes_a_free_box();
    test_rule1_prefers_double_box();
    test_rule2_avoids_giving_boxes();
    test_rules_on_random_boards();
    test_strength_vs_random();
    test_rule3_smallest_sacrifice();
    test_strength_vs_step2();

    printf("\n%d bot tests run, %d failed\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
