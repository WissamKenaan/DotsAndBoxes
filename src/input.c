/*
 * input.c - Reading and parsing a move typed by the player.
 *
 * We read a whole line with fgets() (which never writes past the end
 * of the buffer) and then convert numbers with strtol() (which, unlike
 * scanf("%d"), reports where parsing stopped and handles overflow).
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"

#define LINE_SIZE 128
#define NUMBERS_PER_MOVE 4

/* Discards the rest of a line that was too long to fit in the buffer. */
static void discard_rest_of_line(void)
{
    int ch;
    do {
        ch = getchar();
    } while (ch != '\n' && ch != EOF);
}

InputStatus input_parse_move(const char *line, Move *move)
{
    long values[NUMBERS_PER_MOVE];
    const char *cursor = line;
    char *end = NULL;
    int i;

    for (i = 0; i < NUMBERS_PER_MOVE; i++) {
        values[i] = strtol(cursor, &end, 10);
        if (end == cursor) {
            return INPUT_BAD_FORMAT; /* no digits found */
        }
        /* Any value outside int range is surely out of the board too.
         * Clamp it to -1 so board_validate_move() reports "out of range". */
        if (values[i] < -1000 || values[i] > 1000) {
            values[i] = -1;
        }
        cursor = end;
    }

    /* Only whitespace may follow the fourth number. */
    while (*cursor != '\0') {
        if (!isspace((unsigned char)*cursor)) {
            return INPUT_BAD_FORMAT;
        }
        cursor++;
    }

    move->r1 = (int)values[0];
    move->c1 = (int)values[1];
    move->r2 = (int)values[2];
    move->c2 = (int)values[3];
    return INPUT_OK;
}

InputStatus input_read_move(Move *move)
{
    char line[LINE_SIZE];

    if (fgets(line, sizeof line, stdin) == NULL) {
        return INPUT_EOF;
    }

    /* No newline in the buffer means the line was longer than LINE_SIZE.
     * Throw the rest away so it is not read as the next move. */
    if (strchr(line, '\n') == NULL && !feof(stdin)) {
        discard_rest_of_line();
        return INPUT_BAD_FORMAT;
    }

    return input_parse_move(line, move);
}
