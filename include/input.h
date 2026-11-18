/*
 * input.h - Reading and parsing a move typed by the player.
 */
#ifndef INPUT_H
#define INPUT_H

#include "board.h"

typedef enum {
    INPUT_OK = 0,      /* four integers were read into the Move     */
    INPUT_BAD_FORMAT,  /* line did not contain exactly four integers */
    INPUT_EOF          /* standard input is closed (Ctrl+D / end of file) */
} InputStatus;

/*
 * Reads one line from standard input and parses it as
 * "row1 col1 row2 col2". Only the format is checked here;
 * the game rules are checked by board_validate_move().
 */
InputStatus input_read_move(Move *move);

/*
 * Parses a line of text as four integers. Exposed separately so it
 * can be unit-tested without typing at the keyboard.
 */
InputStatus input_parse_move(const char *line, Move *move);

#endif /* INPUT_H */
