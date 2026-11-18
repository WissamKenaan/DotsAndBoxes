#!/bin/sh
# run_tests.sh - Plays complete scripted games through the real program
# and checks the final result. POSIX sh, so it runs on Alpine's ash.
#
# The expected results were computed by an independent simulator, not
# by this program, so a bug in the C code would show up as a mismatch.

GAME=./dotsboxes
INPUTS=tests/inputs
failures=0

expect_line() {
    # $1 = input file, $2 = text that must appear in the output
    if "$GAME" < "$INPUTS/$1" | grep -qF "$2"; then
        echo "  PASS  $1: found \"$2\""
    else
        echo "  FAIL  $1: missing \"$2\""
        failures=$((failures + 1))
    fi
}

expect_count() {
    # $1 = input file, $2 = text, $3 = how many times it must appear
    count=$("$GAME" < "$INPUTS/$1" | grep -cF "$2")
    if [ "$count" -eq "$3" ]; then
        echo "  PASS  $1: \"$2\" appears $3 times"
    else
        echo "  FAIL  $1: \"$2\" appears $count times, expected $3"
        failures=$((failures + 1))
    fi
}

echo "scripted full games"
expect_line tie_game.txt    "Final score: A = 10, B = 10"
expect_line tie_game.txt    "It's a tie!"
expect_line a_wins_game.txt "Final score: A = 19, B = 1"
expect_line a_wins_game.txt "Player A wins!"
expect_line b_wins_game.txt "Final score: A = 1, B = 19"
expect_line b_wins_game.txt "Player B wins!"

echo "invalid input handling"
expect_count invalid_input.txt "Invalid move:" 11
expect_line  invalid_input.txt "Input closed before the game finished."

"$GAME" < "$INPUTS/invalid_input.txt" > /dev/null
status=$?
if [ "$status" -eq 1 ]; then
    echo "  PASS  invalid_input.txt: exit status 1 on early end of input"
else
    echo "  FAIL  invalid_input.txt: exit status $status, expected 1"
    failures=$((failures + 1))
fi

if [ "$failures" -eq 0 ]; then
    echo "All scripted tests passed."
    exit 0
fi
echo "$failures scripted test(s) failed."
exit 1
