# Debugging Strategy: GDB and Valgrind

## Sprint 2 integrated bot mode

The input fixtures include a menu choice: `1` for existing two-player
fixtures and `2` for `bot_game.txt`. To inspect the real human-vs-bot loop:

```sh
make
gdb --batch -ex "break bot_choose_move" \
  -ex "run < tests/inputs/bot_game.txt > /dev/null" \
  -ex "print bot_player" -ex "backtrace" ./dotsboxes
valgrind --leak-check=full --track-origins=yes --error-exitcode=1 \
  ./dotsboxes < tests/inputs/bot_game.txt > /dev/null
```

At this breakpoint `bot_player` should be `66 'B'`, called from `game_run`.
The supplied bot-unit-test GDB screenshot uses A and is separate evidence;
see [SPRINT2_REVIEW.md](SPRINT2_REVIEW.md) for the unchanged originals.
The remaining examples below document the earlier Sprint 1 sessions, so
source line numbers and addresses can differ after integration.

The Makefile always compiles with `-g -O0`:

- `-g` puts debug symbols (function names, line numbers, variable names) into
  the binary, so GDB and Valgrind can point to exact lines of source code.
- `-O0` turns off optimisation, so every variable can be printed in GDB and the
  program runs line by line in the order you wrote it.

The outputs below come from real runs of this code. They were first run on
Ubuntu (glibc) and then repeated on the Alpine Linux 3.24 VM; the Alpine
screenshots are in `docs/screenshots/`.

---

## Part 1 — GDB

### Starting GDB with a scripted game

The game reads moves from the keyboard. Inside GDB you can feed it a move file
instead, so every debugging session is repeatable:

```sh
make
gdb ./dotsboxes
(gdb) run < tests/inputs/tie_game.txt > /dev/null
```

### Breakpoints and inspecting variables

```
(gdb) break board_apply_move
Breakpoint 1 at 0x17e3: file src/board.c, line 143.
(gdb) run < tests/inputs/tie_game.txt > /dev/null

Breakpoint 1, board_apply_move (board=0x7fffffff9ac0, move=..., player=65 'A') at src/board.c:143
143	    Move m = normalize_move(move);
(gdb) print move
$1 = {r1 = 4, c1 = 4, r2 = 4, c2 = 5}
(gdb) print player
$2 = 65 'A'
```

The first move in `tie_game.txt` is `4 4 4 5`, and it is Player A's turn. This
confirms the input was read and passed on correctly.

### Following execution

| Command        | What it does                                         |
|----------------|------------------------------------------------------|
| `next` (`n`)   | run the current line, without entering functions     |
| `step` (`s`)   | run the current line, entering any function it calls |
| `finish`       | run until the current function returns, then show the return value |
| `continue` (`c`) | run until the next breakpoint                      |
| `backtrace` (`bt`) | show the chain of function calls                 |

```
(gdb) next
144	    int completed = 0;
(gdb) next
148	    if (is_horizontal(m)) {
(gdb) print m
$3 = {r1 = 4, c1 = 4, r2 = 4, c2 = 5}
(gdb) finish
Run till exit from #0  board_apply_move (...)
game_play_move (game=0x7fffffff9ac0, move=...) at src/game.c:23
Value returned is $4 = 0
```

`finish` shows that the first move completed 0 boxes, which is correct.

### Conditional breakpoints

To stop only near the end of the game, when 19 boxes are already claimed:

```
(gdb) delete 1
(gdb) break try_claim_box if board->claimed_boxes == 19
(gdb) continue

Breakpoint 2, try_claim_box (board=0x7fffffff9ac0, box_row=-1, box_col=3, player=66 'B') at src/board.c:127
(gdb) backtrace
#0  try_claim_box (board=..., box_row=-1, box_col=3, player=66 'B') at src/board.c:127
#1  board_apply_move (board=..., move=..., player=66 'B') at src/board.c:150
#2  game_play_move (game=..., move=...) at src/game.c:23
#3  game_run () at src/game.c:95
#4  main () at src/main.c:8
(gdb) print board->owner
$7 = {"BAA A", "BBBAA", "ABAAA", "ABBBB"}
```

`print board->owner` prints the whole ownership grid. Exactly one box (row 0,
column 3) is still empty, which matches `claimed_boxes == 19`.
`box_row = -1` shows the "box above row 0" case being checked; `try_claim_box`
correctly rejects it as off the board.

### Finding and fixing a real logic error with GDB

**The bug.** In `board_apply_move`, the box *above* a horizontal line was
checked with the wrong row:

```c
completed += try_claim_box(board, m.r1, m.c1, player);     /* box above (WRONG) */
```

instead of `m.r1 - 1`. When a box's **bottom** side was drawn last, the box was
never awarded.

**1. Reproduce.** `make test` reported:

```
FAIL  bottom side drawn last completes the box above it  (tests/test_board.c:143)
45 tests run, 1 failed
```

**2. Isolate in GDB.** The failing test draws the bottom line `2 2 2 3` of box
(1,2) as Player B. A conditional breakpoint stops on exactly that move:

```
$ gdb ./test_board
(gdb) break board_apply_move if move.r1 == 2 && move.c1 == 2 && move.r2 == 2 && move.c2 == 3 && player == 66
(gdb) run
Breakpoint 1, board_apply_move (board=..., move=..., player=66 'B') at src/board.c:143
(gdb) break try_claim_box
(gdb) continue
Breakpoint 2, try_claim_box (board=..., box_row=2, box_col=2, player=66 'B') at src/board.c:127
(gdb) continue
Breakpoint 2, try_claim_box (board=..., box_row=2, box_col=2, player=66 'B') at src/board.c:127
```

(`66` is the ASCII code of `'B'`.) A horizontal line from (2,2) to (2,3)
touches box (1,2) above it and box (2,2) below it. GDB shows **both** calls
checking `box_row = 2`, so box (1,2) was never checked. The bug is in the
"box above" calculation.

**3. Fix.** Change the first call to `m.r1 - 1`, rebuild, and run `make test`
again: `45 tests run, 0 failed`.

This bug really was planted and caught during development. The test that
catches it (`test_each_side_last`) was written because the first tests only
ever completed boxes with vertical lines, which would have hidden it.

---

## Part 2 — Valgrind

```sh
make valgrind
```

This runs:

```sh
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
         --error-exitcode=1 ./dotsboxes < tests/inputs/tie_game.txt > /dev/null
```

| Flag                       | What it checks                                         |
|----------------------------|--------------------------------------------------------|
| `--leak-check=full`        | memory that was allocated but never freed              |
| `--show-leak-kinds=all`    | every kind of leak (definite, indirect, possible, reachable) |
| `--track-origins=yes`      | for an uninitialised value, where it came from         |
| `--error-exitcode=1`       | makes the command fail if Valgrind finds any error     |

Invalid reads/writes (out-of-bounds array access) are always reported, with no
extra flag needed.

### Results (full 49-move games and invalid-input run)

```
==300==     in use at exit: 0 bytes in 0 blocks
==300==   total heap usage: 2 allocs, 2 frees, 8,192 bytes allocated
==300== All heap blocks were freed -- no leaks are possible
==300== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

The same result was obtained for `a_wins_game.txt`, `invalid_input.txt`, and
the unit test binary `./test_board`.

**What the 2 allocations are:** the program itself never calls `malloc`. All
game state is in fixed-size arrays inside a `Game` struct on the stack. The 2
allocations are the C library's own buffers for `stdin`/`stdout`, and both are
freed at exit. Because the program does no dynamic allocation, leaks are
avoided by design; Valgrind confirms it.

**On Alpine (verified on the VM):** Alpine uses the musl C library instead of
glibc. There, Valgrind reports `total heap usage: 0 allocs, 0 frees, 0 bytes
allocated`, together with `All heap blocks were freed` and `0 errors`. musl
doesn't allocate stdio buffers on the heap, so this confirms that our own code
makes no heap allocations at all. See `docs/screenshots/01_make_test_and_valgrind.png`.

### Valgrind on the unit tests and other inputs

```sh
valgrind --leak-check=full --error-exitcode=1 ./test_board
valgrind --leak-check=full --error-exitcode=1 ./dotsboxes < tests/inputs/invalid_input.txt
```

### What a real error would look like (for the presentation)

To test this, the line `board->claimed_boxes = 0;` was temporarily commented
out of `board_init`. Valgrind (`--track-origins=yes`) reported:

```
==452== Conditional jump or move depends on uninitialised value(s)
==452==    at 0x10A1B5: game_run (game.c:90)
==452==    by 0x10A1ED: main (main.c:8)
==452==  Uninitialised value was created by a stack allocation
==452==    at 0x10A0D7: game_run (game.c:83)
```

How to read it:
- `game.c:90` is `while (!game_is_over(&game))`. The loop decision depends on
  a value that was never set.
- `game.c:83` is the start of `game_run`, where the `Game game;` local variable
  lives. `--track-origins=yes` is what adds this second part, pointing to the
  memory the bad value came from.
- Following the call chain `game_is_over` → `board_is_full` →
  `claimed_boxes` leads to the missing initialisation.

The line was then restored, and Valgrind reported 0 errors again.
