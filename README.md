# Dots and Boxes — CMPS 241 Course Project (Sprints 1–2)

A console-based Dots and Boxes game written in C, with two-player and human-vs-bot modes, built to run on
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

Choose `1` at the start menu for two human players, or `2` to play against
the bot. In bot mode you are Player A and the bot is Player B. Invalid menu
choices are asked again. Ctrl+D at the menu exits successfully; Ctrl+D during
an unfinished game exits with status 1.

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
make test       # 49 board/game checks + 20 bot checks + 9 scripted checks
make test-game  # 107 end-to-end scenarios; requires python3 (apk add python3)
python3 tests/test_modes.py  # 38 menu/bot scenarios, including 30 full games
make valgrind   # full scripted game under Valgrind (exit code 1 on any error)
```

See [docs/TESTING.md](docs/TESTING.md) and [docs/DEBUGGING.md](docs/DEBUGGING.md).
The [Sprint 2 review](docs/SPRINT2_REVIEW.md) contains the work split and supplied
Alpine screenshots; [bot strategy](docs/BOT_STRATEGY.md) describes Wissam's algorithm.

## Project structure

```
DotsAndBoxes/
├── include/            Header files (public interface of each module)
│   ├── board.h           board data model and game rules
│   ├── display.h         all console output
│   ├── input.h           reading and parsing a move
│   ├── bot.h             bot move-selection interface
│   └── game.h            turns, scores, game loop
├── src/                Implementation files
│   ├── board.c
│   ├── display.c
│   ├── input.c
│   ├── game.c
│   ├── bot.c             capture, safety, sacrifice and chain-control strategy
│   └── main.c            entry point
├── tests/
│   ├── test_board.c      unit tests
│   ├── test_bot.c        bot strategy tests
│   ├── test_game.py      two-player reference-model checks
│   ├── test_modes.py     menu and interactive human-vs-bot checks
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

The configured profile starts the installed game at boot. Follow
[docs/ALPINE_SETUP.md](docs/ALPINE_SETUP.md), reinstall after pulling changes,
and verify that tty1 shows the new mode menu. The supplied Sprint 2 bot
screenshots do not establish that this updated menu boots automatically.
