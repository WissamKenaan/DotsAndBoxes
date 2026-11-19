# Dots and Boxes — CMPS 241 Course Project (Sprint 1)

A console-based, two-player Dots and Boxes game written in C, built to run on
Alpine Linux inside a virtual machine and to start automatically at boot.

## Game rules (Sprint 1)

- The board has **5 rows × 6 columns of dots**, forming a **4 × 5 grid of 20 boxes**.
- Player **A** moves first. Players take turns drawing one line between two
  dots that are next to each other horizontally or vertically.
- A player who draws the fourth side of a box claims it; their initial
  (`A` or `B`) appears inside it, and they **take another turn**.
- One line can complete **two boxes at once**; the player gets both.
- When all 20 boxes are claimed, the player with more boxes wins; 10–10 is a tie.

## How to enter a move

Type four numbers: `row1 col1 row2 col2`, then press Enter.

```
Player A's turn. Enter the row and column of the first dot (e.g., A0 -> 0 0) and second dot:
0 4 0 3
```

This connects dot (row 0, col 4) to dot (row 0, col 3). The dots can be given
in either order. Invalid input is rejected with a message, and the same player
is asked again.

## Requirements

On Alpine Linux:

```sh
apk add build-base gdb valgrind git
```

`build-base` provides `gcc`, `make` and the C library headers.

## Build and run

```sh
make            # builds ./dotsboxes
./dotsboxes     # or: make run
```

## Testing and debugging

```sh
make test       # 49 unit tests + 9 scripted full-game checks
make test-game  # 107 end-to-end scenarios; requires python3 (apk add python3)
make valgrind   # full scripted game under Valgrind (exit code 1 on any error)
```

See [docs/TESTING.md](docs/TESTING.md) and [docs/DEBUGGING.md](docs/DEBUGGING.md).

## Project structure

```
DotsAndBoxes/
├── include/            Header files (public interface of each module)
│   ├── board.h           board data model and game rules
│   ├── display.h         all console output
│   ├── input.h           reading and parsing a move
│   └── game.h            turns, scores, game loop
├── src/                Implementation files
│   ├── board.c
│   ├── display.c
│   ├── input.c
│   ├── game.c
│   └── main.c            entry point
├── tests/
│   ├── test_board.c      unit tests
│   ├── run_tests.sh      scripted full-game tests
│   └── inputs/           move files: tie, A wins, B wins, invalid input
├── scripts/
│   └── player_profile    auto-start script for the VM
├── docs/
│   ├── Sprint1_Report.pdf  Sprint 1 report (also .docx and REPORT.md)
│   ├── DESIGN.md         requirements analysis and architecture
│   ├── ALPINE_SETUP.md   VM installation and auto-start at boot
│   ├── DEBUGGING.md      GDB and Valgrind procedures
│   ├── TESTING.md        test cases and results
│   └── GITHUB.md         repository setup and commit plan
├── Makefile
└── README.md
```

## Auto-start on the Alpine VM

The game starts on boot with no username or password prompt. The steps are
in [docs/ALPINE_SETUP.md](docs/ALPINE_SETUP.md).
