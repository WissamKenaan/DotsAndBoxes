/*
 * display.c - Console output for Dots and Boxes.
 *
 * Board layout (matches the project sample run):
 *
 *     0 1 2 3 4 5        <- column numbers
 *   0 .-.-. . . .        <- dot row: '-' between dots = horizontal line
 *     |A| |              <- gap row: '|' under a dot = vertical line,
 *   1 .-.-. . . .           letter between bars = owner of the box
 */
#include <stdio.h>
#include "display.h"

#define SEPARATOR_LENGTH 80

void display_menu(void)
{
    printf("Dots and Boxes\n"
           "1) Two players\n"
           "2) Play against the bot\n"
           "Choose 1 or 2:\n");
    fflush(stdout);
}

void display_invalid_choice(void)
{
    printf("Invalid choice. Enter 1 or 2.\n");
}

void display_menu_closed(void)
{
    printf("\nInput closed at the menu. Exiting.\n");
    fflush(stdout);
}

void display_bot_move(Move move)
{
    printf("Bot (Player B) plays: %d %d %d %d\n",
           move.r1, move.c1, move.r2, move.c2);
    fflush(stdout);
}

static void print_separator(void)
{
    int i;
    for (i = 0; i < SEPARATOR_LENGTH; i++) {
        putchar('*');
    }
    putchar('\n');
}

/* Prints one row of dots with any horizontal lines between them. */
static void print_dot_row(const Board *board, int row)
{
    int c;

    printf("%d ", row);
    for (c = 0; c < DOT_COLS; c++) {
        putchar('.');
        if (c < DOT_COLS - 1) {
            putchar(board->horizontal[row][c] ? '-' : ' ');
        }
    }
    putchar('\n');
}

/* Prints the row between dot rows `row` and `row + 1`: vertical lines
 * and the initials of the owners of the boxes in that row. */
static void print_gap_row(const Board *board, int row)
{
    int c;

    printf("  ");
    for (c = 0; c < DOT_COLS; c++) {
        putchar(board->vertical[row][c] ? '|' : ' ');
        if (c < BOX_COLS) {
            putchar(board->owner[row][c]);
        }
    }
    putchar('\n');
}

void display_board(const Board *board)
{
    int r;
    int c;

    printf("  ");
    for (c = 0; c < DOT_COLS; c++) {
        printf("%d", c);
        if (c < DOT_COLS - 1) {
            putchar(' ');
        }
    }
    putchar('\n');

    for (r = 0; r < DOT_ROWS; r++) {
        print_dot_row(board, r);
        if (r < DOT_ROWS - 1) {
            print_gap_row(board, r);
        }
    }
}

void display_scores(int score_a, int score_b)
{
    print_separator();
    printf("Player A score: %d\n", score_a);
    printf("Player B score: %d\n", score_b);
    print_separator();
}

void display_turn_prompt(char player)
{
    printf("Player %c's turn. Enter the row and column of the first dot "
           "(e.g., A0 -> 0 0) and second dot:\n", player);
    fflush(stdout);
}

void display_invalid_move(const char *reason)
{
    printf("Invalid move: %s Please try again.\n", reason);
}

void display_result(int score_a, int score_b)
{
    printf("Game over! Final score: A = %d, B = %d\n", score_a, score_b);
    if (score_a > score_b) {
        printf("Player A wins!\n");
    } else if (score_b > score_a) {
        printf("Player B wins!\n");
    } else {
        printf("It's a tie!\n");
    }
    fflush(stdout);
}

void display_input_closed(void)
{
    printf("\nInput closed before the game finished. Exiting.\n");
    fflush(stdout);
}
