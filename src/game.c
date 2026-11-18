/*
 * game.c - Turn management, scoring, and the main game loop.
 */
#include "game.h"
#include "display.h"
#include "input.h"

void game_init(Game *game)
{
    board_init(&game->board);
    game->score_a = 0;
    game->score_b = 0;
    game->current_player = PLAYER_A;
}

static char other_player(char player)
{
    return player == PLAYER_A ? PLAYER_B : PLAYER_A;
}

int game_play_move(Game *game, Move move)
{
    int completed = board_apply_move(&game->board, move, game->current_player);

    if (game->current_player == PLAYER_A) {
        game->score_a += completed;
    } else {
        game->score_b += completed;
    }

    if (completed == 0) {
        game->current_player = other_player(game->current_player);
    }
    return completed;
}

int game_is_over(const Game *game)
{
    return board_is_full(&game->board);
}

char game_winner(const Game *game)
{
    if (game->score_a > game->score_b) {
        return PLAYER_A;
    }
    if (game->score_b > game->score_a) {
        return PLAYER_B;
    }
    return 0;
}

/*
 * Keeps asking the current player for a move until a legal one is
 * entered. Returns 1 when `move` holds a legal move, 0 on end of input.
 */
static int read_legal_move(const Game *game, Move *move)
{
    for (;;) {
        InputStatus input_status;
        MoveStatus move_status;

        display_turn_prompt(game->current_player);
        input_status = input_read_move(move);

        if (input_status == INPUT_EOF) {
            return 0;
        }
        if (input_status == INPUT_BAD_FORMAT) {
            display_invalid_move("Enter exactly four numbers, e.g. 0 0 0 1.");
            continue;
        }

        move_status = board_validate_move(&game->board, *move);
        if (move_status == MOVE_OK) {
            return 1;
        }
        display_invalid_move(board_status_message(move_status));
    }
}

int game_run(void)
{
    Game game;
    Move move = {0, 0, 0, 0};

    game_init(&game);
    display_board(&game.board);

    while (!game_is_over(&game)) {
        if (!read_legal_move(&game, &move)) {
            display_input_closed();
            return 1;
        }
        game_play_move(&game, move);
        display_scores(game.score_a, game.score_b);
        display_board(&game.board);
    }

    display_result(game.score_a, game.score_b);
    return 0;
}
