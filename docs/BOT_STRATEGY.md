# Hard-Level Bot: Strategy and Complexity (Sprint 2)

Code: `include/bot.h`, `src/bot.c`. Tests: `tests/test_bot.c` (run by `make test`).

## 1. Interface

```c
Move bot_choose_move(const Board *board, char bot_player);
```

The bot only **reads** the board (`const`). It returns a move, and the game
plays it exactly like a human move. Every candidate move is taken from the list
of free lines, so the bot **can never play an invalid or repeated line**. Two
tests check this: a complete bot-vs-bot game, and 2,000 random partly-filled
boards, on each of which the move must pass `board_validate_move()`.

## 2. Background: chains and control

Near the end of a game, every free line gives something away: drawing it leaves
a box with 3 sides. The remaining boxes then form **chains**, rows of boxes
where each box gives the next one away. The player who must open a chain
usually loses it, because the opponent takes the whole chain, one box at a
time, with an extra turn after each box.

A player who is eating a chain can decline its last two boxes on purpose. This
is called **double-dealing**, and it hands the opponent 2 boxes but forces them
to open the next chain. The player who controls who opens each chain wins the
endgame. Our bot is built around this idea.

## 3. The strategy (rules in priority order)

| Rule | When | What the bot does |
|---|---|---|
| **1. Capture** | a line completes a box | draws it, preferring a line that completes **two** boxes |
| **4. Keep control** | exactly 2 boxes are left in a row, and no safe line is left anywhere | compares "take both" with "decline both" by looking ahead to the end of the game, and picks the better one |
| **2. Safe move** | no capture | draws a line that leaves no box with 3 sides, so the opponent gets nothing |
| **3. Smallest sacrifice** | every line gives something away | draws the line after which the opponent can take the **fewest** boxes. On a tie, it prefers a line that does **not** let the opponent double-deal back (it opens a 2-box chain in the middle, not at its end) |

### How each rule decides (main functions)

- `boxes_completed_by(board, move)` tries the move on a **copy** of the board
  and returns how many boxes it completes. The real board is never modified.
- `gives_away_box(board, move)` tries the move on a copy and checks whether any
  box is left with exactly 3 sides.
- `boxes_given_away(board, move)` plays the bot's move on a copy, then plays as
  the opponent, capturing greedily while any capture exists, and counts the
  boxes taken. This is the size of the chain the move would open.
- `find_double_deal(board, &deal)` detects the exact moment for Rule 4: one
  box X with 3 sides, whose last side gives box Y its 3rd side, and nothing
  capturable after Y. It returns the far side of Y. Drawing that line completes
  nothing and leaves X and Y both with 3 sides, sharing one free line.
- `endgame_value(board)` looks ahead to the end of the game, assuming both
  players follow this same strategy. It returns the final score difference for
  the player to move. It only branches at double-dealing points ("take" or
  "decline"), so the search stays small.

### Example (Rule 4)

All horizontal lines are drawn, so each row of boxes is a chain of 5 (4 chains).
The opponent opened row 0, and the bot has taken 3 of its 5 boxes:

- **Take both:** +2 now, but the bot must then open one of the three chains of 5
  for the opponent.
- **Decline both:** the bot draws the far end `(0,5)-(1,5)`. The opponent takes
  2 boxes with one line, then must open a chain of 5 for the bot, and so on.

Test `test_rule4_keeps_control` checks that the bot declines here, and
`test_rule4_takes_all_when_last_chain` checks that it takes both boxes when no
chain is left after them.

## 4. Complexity analysis

Notation, for a board of R × C boxes:

| Symbol | Meaning | Our board |
|---|---|---|
| E | number of lines, R(C+1) + C(R+1) | 49 |
| B | number of boxes, R·C | 20 |
| L | free lines in the current position (L ≤ E) | 0–49 |
| k | double-dealing points in the endgame (at most one per chain) | small |

Basic operations:

| Operation | Cost | Why |
|---|---|---|
| `board_apply_move`, box-completion check | O(1) | a line touches at most 2 boxes (Sprint 1 design) |
| copying a `Board` | O(E) | fixed-size arrays of lines and boxes |
| `list_legal_moves` | O(E) | scans every line once |
| `boxes_completed_by`, `gives_away_box` | O(E) | one board copy, plus a scan of B ≤ E boxes |

Cost of each rule, for one bot move:

| Rule | Cost | Derivation |
|---|---|---|
| 1. Capture (`find_best_capture`) | O(L·E) ⊆ O(E²) | L candidates × O(E) each |
| 2. Safe move | O(L·E) ⊆ O(E²) | L candidates × `gives_away_box` |
| `boxes_given_away` (one candidate) | O(B·E²) | at most B+1 rounds, each scanning L lines × O(E) |
| 3. Smallest sacrifice (`choose_sacrifice`) | O(L·B·E²) ⊆ O(B·E³) | L candidates × (`boxes_given_away` + `find_double_deal` = O(E²)) |
| 4. Keep control (`endgame_value`) | O(2^k · k · B·E³) | see below |

**Look-ahead (`endgame_value`).** One simulation of the rest of the game draws
at most L lines:
- each capture costs O(E²);
- each of the at most k sacrifices costs O(B·E³);

so one simulation costs O(k·B·E³). The search branches only at
double-dealing points, into "take" and "decline". Along any path there are at
most k such points, so there are at most 2^k simulations.

**Total per bot move (worst case):**

T(E, B, k) = O(2^k · k · B · E³)

**Space:** the recursion is at most L ≤ E levels deep, and each level stores
one `Board` copy and one list of free lines, each O(E). Total space is O(E²).
The bot uses only local variables (no `malloc`), and Valgrind reports 0 errors
and no leaks on the bot tests.

**Why not a full game-tree search (minimax over all moves)?** A full search
from the start would explore about L! move orders (49! ≈ 6 × 10⁶²), which is
impossible. Our strategy is polynomial in the board size. Its only exponential
factor is 2^k, over the number of *chains* rather than lines. Chains are few,
because each one holds several boxes.

**Measured on our board (4 × 5 boxes), over 400 test games:**

- slowest single bot move: **0.002 s**;
- largest look-ahead for one move: **200** calls to `endgame_value`;
- the look-ahead ran on 456 of the bot's moves, and was not needed on all the
  others.

## 5. Results (tests/test_bot.c, 20 checks)

| Test | Result |
|---|---|
| Legal moves: full bot-vs-bot game, 2,000 random boards, last free line | all legal |
| Rule 1 on 3,000 random boards | always takes the biggest capture |
| Rule 2 on the same boards | always plays safe when it can |
| Rule 3 on 300 endgame boards | always gives the minimum possible |
| Rule 3 tie-break on 300 endgame boards | never lets the opponent double-deal when this can be avoided |
| Rule 4 hand-built positions | declines with chains left; takes the last chain |
| 200 games against a random player | **200 wins**, 0 ties, 0 losses |
| 400 games against the Step 2 bot (Rules 1–2) | **385 wins**, 4 ties, 11 losses |
| 400 games against the Step 3 bot (Rules 1–3) | **349 wins**, 8 ties, 43 losses |

Each benchmark game starts with 8–22 random safe lines, so every game is
different. The random generator has a fixed seed, so the results can be
reproduced exactly.

## 6. Limitations

- **Loops** (closed chains) are captured greedily. The expert play of leaving
  4 boxes at the end of a loop is not implemented.
- The look-ahead assumes that the opponent follows the same strategy. Against
  a different opponent, it is an estimate, not a proof.
- Among several equally good safe lines, the bot takes the first one in board
  order, so its opening is predictable.

## 7. Debugging the bot with GDB

`make` builds with `-g -O0`, so the bot's decision can be inspected:

```
$ gdb ./test_bot
(gdb) break bot.c:309          # the line "return deal;" in bot_choose_move
(gdb) run
Breakpoint 1, bot_choose_move (board=..., bot_player=65 'A') at src/bot.c:309
(gdb) print value_take
$1 = -5
(gdb) print value_decline
$2 = 5
(gdb) print deal
$3 = {r1 = 0, c1 = 5, r2 = 1, c2 = 5}
(gdb) backtrace
#0  bot_choose_move (...) at src/bot.c:309
#1  test_bot_plays_whole_game_legally () at tests/test_bot.c:68
#2  main () at tests/test_bot.c:885
```

During the bot-vs-bot game, the look-ahead found that taking the last two
boxes would end the game 5 boxes behind, while declining them ends it
5 boxes ahead, so the bot declines.
