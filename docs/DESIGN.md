# Design Document — Sprint 1

## 1. Requirements (from the project PDF, pages 1–8)

### Functional

| ID  | Requirement | Source |
|-----|-------------|--------|
| F1  | Display a 4×5 grid of boxes (5 rows × 6 columns of dots, numbered 0–4 and 0–5) | §3.1 "Requirements"; board images p.1 |
| F2  | Two players, A and B, take turns connecting dots | §3.1 |
| F3  | A move is entered as `row col row col` | sample run p.5–8 |
| F4  | Update and redisplay the board after each move | §3.1 |
| F5  | Detect when a move completes a box and award it; show the player's initial inside | §3.1 description |
| F6  | Show both scores after every move | sample run p.5–8 |
| F7  | Declare the winner when all boxes are claimed | §3.1 |

### Non-functional

| ID  | Requirement |
|-----|-------------|
| N1  | Console only; runs on a small Linux without a GUI (Alpine) |
| N2  | Runs inside a VM and launches automatically at boot, with no login prompt |
| N3  | Code split into multiple `.h` and `.c` files, built with a Makefile |
| N4  | Debugged with GDB and Valgrind |
| N5  | Everything on GitHub, with regular, meaningful commits |
| N6  | A program that does not compile receives zero |
| N7  | Report + oral presentation; 50% for report/presentation/GitHub/debugging, 50% for correctness |

## 2. Behaviour taken from the sample run

| Observation in the screenshots | Implemented as |
|---|---|
| `0 4 0 3` draws the line between (0,3) and (0,4): the dots can be given in either order | Moves are normalised (smaller dot first) before use |
| After A completes a box with `0 3 1 3`, the next prompt is **"Player A's turn"** again | The player who completes a box moves again |
| Same for B (`1 4 1 5`) and A (`2 2 3 2`, then `3 1 3 2`) | Confirms the extra-turn rule |
| Score block between two lines of `*` after every move | `display_scores()` |
| Initial board printed with no score block | `game_run()` prints only the board first |
| Gap rows: `|` under dots for vertical lines, letter between bars for an owned box | `print_gap_row()` |

The extra-turn rule is also the standard Dots and Boxes rule, so the sample
and the standard rules agree.

## 3. Ambiguities and decisions

| # | Ambiguity | Decision and reason |
|---|-----------|---------------------|
| 1 | The prompt says `(e.g., A0 -> 0 0)`, but every input in the sample is four numbers | The prompt text is kept exactly as shown, but only numeric input is accepted. `A0`-style input is rejected with a clear message. |
| 2 | "4x5 grid of boxes": is it 4 rows × 5 columns or the reverse? | 4 rows × 5 columns, matching the 5×6 dots in every screenshot |
| 3 | What happens on invalid input is not shown | Print `Invalid move: <reason>`; the **same** player tries again; the board does not change |
| 4 | The end-of-game screen is not shown | Final board, final score, then `Player A wins!` / `Player B wins!` / `It's a tie!` |
| 5 | Ties are not mentioned (20 boxes allow 10–10) | Handled explicitly as a tie |
| 6 | The input stream ending (Ctrl+D) is not mentioned | Print a message and exit with status 1, so the program never loops forever |
| 7 | What the VM shows after a game ends is not specified | The auto-start script waits for Enter, then starts a new game |

## 4. Architecture

```
            main.c
              │
            game.c   ── turn order, scores, game loop
           ╱  │   ╲
   input.c  board.c  display.c
   (parse)  (rules)  (printing)
```

| Module | Responsibility | Depends on |
|--------|----------------|------------|
| `board` | Board state, move validation, box completion | nothing (no I/O at all) |
| `input` | Read one line safely, parse four integers | `board.h` (for `Move`) |
| `display` | All `printf` calls | `board.h` |
| `game` | Turn order, scores, winner, main loop | all three |
| `main` | Entry point | `game.h` |

**Why this split:** the rules (`board.c`) do no input or output, so they can be
unit-tested directly, and they can be reused unchanged when Sprint 2 adds a
bot. The output format can change without touching the rules.

## 5. Data model

```c
int  horizontal[5][5];  /* horizontal[r][c]: (r,c) — (r,c+1) */
int  vertical[4][6];    /* vertical[r][c]:   (r,c) — (r+1,c) */
char owner[4][5];       /* 'A', 'B', or ' ' */
int  claimed_boxes;
```

Total: 25 horizontal + 24 vertical = **49 lines**, 20 boxes.

Box `(i, j)` is complete when:

```
horizontal[i][j] && horizontal[i+1][j] && vertical[i][j] && vertical[i][j+1]
```

**Memory:** everything is in fixed-size arrays inside one `Game` struct on the
stack. There is no `malloc`, so leaks are impossible by construction, and the
array bounds are checked before every access.

## 6. Algorithms and complexity

| Operation | Approach | Time |
|-----------|----------|------|
| Validate a move | range check, distance check (\|Δr\|+\|Δc\| = 1), lookup in one array | O(1) |
| Detect completed boxes | a line touches at most 2 boxes; check only those 2, 4 lookups each | **O(1)** |
| Game over | compare `claimed_boxes` with 20 | O(1) |
| Print board | visit every dot and line | O(R·C) |
| Whole game | 49 moves × O(1) + 49 prints | O(R·C) per move |

A naive version would scan all 20 boxes after each move (O(R·C)). Checking only
the two neighbouring boxes is the key design decision. The project description
asks for exactly this kind of complexity analysis.

## 7. Input safety

- `fgets` reads at most 127 characters, so it can never overflow the buffer.
  Longer lines are discarded completely.
- `strtol` (not `scanf("%d")`) detects missing numbers and extra text, and does
  not have undefined behaviour on huge numbers.
- Exactly four integers followed only by whitespace are accepted.

## 8. Implementation plan (phases)

1. `board.h/.c` and their unit tests → commit
2. `display.h/.c` → check the output against the screenshots → commit
3. `input.h/.c` and parsing tests → commit
4. `game.h/.c`, `main.c`, playable game → commit
5. Makefile targets `test` / `valgrind`, scripted full games → commit
6. GDB/Valgrind sessions on Alpine → screenshots for the report → commit docs
7. VM auto-start configuration → reboot test → commit docs
