"""Check complete console games against an independent set-based model."""

from pathlib import Path
import random
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
GAME = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else ROOT / "dotsboxes"
HEADER = "  0 1 2 3 4 5"


def edge(first, second):
    return frozenset((first, second))


def box_sides(row, col):
    corners = [(row, col), (row, col + 1), (row + 1, col + 1), (row + 1, col)]
    return {edge(corners[i], corners[(i + 1) % 4]) for i in range(4)}


BOXES = {(r, c): box_sides(r, c) for r in range(4) for c in range(5)}
MOVES = [(r, c, r, c + 1) for r in range(5) for c in range(5)]
MOVES += [(r, c, r + 1, c) for r in range(4) for c in range(6)]


def read_board(lines):
    """Decode the console picture into edges and claimed boxes."""
    drawn = set()
    owners = {}
    assert len(lines) == 9, "incomplete displayed board"
    for row in range(5):
        dots = lines[row * 2]
        assert len(dots) == 13 and dots[:2] == f"{row} ", "incorrect dot row"
        assert all(dots[2 + col * 2] == "." for col in range(6)), "missing dot"
        for col in range(5):
            char = dots[3 + col * 2]
            assert char in " -", "invalid horizontal line character"
            if char == "-":
                drawn.add(edge((row, col), (row, col + 1)))
        if row == 4:
            continue
        gap = lines[row * 2 + 1]
        assert len(gap) == 13 and gap[:2] == "  ", "incorrect box row"
        for col in range(6):
            char = gap[2 + col * 2]
            assert char in " |", "invalid vertical line character"
            if char == "|":
                drawn.add(edge((row, col), (row + 1, col)))
        for col in range(5):
            owner = gap[3 + col * 2]
            assert owner in " AB", "invalid box owner"
            if owner != " ":
                owners[(row, col)] = owner
    return drawn, owners


def check_game(label, moves, invalid_inputs=(), trailing_newline=True):
    drawn = set()
    owners = {}
    player = "A"
    expected_boards = [(set(), {})]
    expected_scores = []
    expected_prompts = []
    input_lines = []
    invalid_count = 0
    captures = {0: 0, 1: 0, 2: 0}

    for index, move in enumerate(moves):
        if index % 7 == 0:
            rejected = list(invalid_inputs)
            if index:
                previous = moves[index - 1].split()
                rejected.append(" ".join(previous[2:] + previous[:2]))
            input_lines.extend(rejected)
            expected_prompts.extend([player] * len(rejected))
            invalid_count += len(rejected)

        expected_prompts.append(player)
        input_lines.append(move)
        r1, c1, r2, c2 = map(int, move.split())
        assert 0 <= r1 < 5 and 0 <= r2 < 5 and 0 <= c1 < 6 and 0 <= c2 < 6
        assert abs(r1 - r2) + abs(c1 - c2) == 1, "invalid test move"
        line = edge((r1, c1), (r2, c2))
        assert line not in drawn, "duplicate test move"
        drawn.add(line)

        # Scan every box using geometric edges, independently of board.c's
        # neighbouring-box lookup and horizontal/vertical array representation.
        completed = [box for box, sides in BOXES.items() if box not in owners and sides <= drawn]
        for box in completed:
            owners[box] = player
        captures[len(completed)] += 1
        expected_scores.append((list(owners.values()).count("A"), list(owners.values()).count("B")))
        expected_boards.append((drawn.copy(), owners.copy()))
        if not completed:
            player = "B" if player == "A" else "A"

    finished = len(owners) == 20
    if not finished:
        expected_prompts.append(player)
    payload = "1\n" + "\n".join(input_lines)
    if trailing_newline and input_lines:
        payload += "\n"
    result = subprocess.run(
        [str(GAME)], input=payload, text=True, capture_output=True, timeout=10, cwd=ROOT
    )
    assert result.returncode == (0 if finished else 1), f"{label}: exit {result.returncode}"
    assert not result.stderr, f"{label}: unexpected stderr {result.stderr}"
    lines = result.stdout.splitlines()
    boards = [read_board(lines[i + 1:i + 10]) for i, line in enumerate(lines) if line == HEADER]
    assert len(boards) == len(expected_boards), f"{label}: incorrect number of board updates"
    for index, (actual, expected) in enumerate(zip(boards, expected_boards)):
        assert actual == expected, f"{label}: board/ownership mismatch after move {index}"
    scores_a = re.findall(r"^Player A score: (\d+)$", result.stdout, re.MULTILINE)
    scores_b = re.findall(r"^Player B score: (\d+)$", result.stdout, re.MULTILINE)
    assert list(zip(map(int, scores_a), map(int, scores_b))) == expected_scores, f"{label}: score mismatch"
    prompts = re.findall(r"^Player ([AB])'s turn\.", result.stdout, re.MULTILINE)
    assert prompts == expected_prompts, f"{label}: incorrect turn or extra turn"
    assert sum(line.startswith("Invalid move:") for line in lines) == invalid_count, f"{label}: rejected input count"
    if finished:
        score_a, score_b = expected_scores[-1]
        winner = "It's a tie!" if score_a == score_b else f"Player {'A' if score_a > score_b else 'B'} wins!"
        assert lines[-2:] == [f"Game over! Final score: A = {score_a}, B = {score_b}", winner], f"{label}: wrong result"
    else:
        assert lines[-1] == "Input closed before the game finished. Exiting.", f"{label}: missing EOF message"
        assert "Game over!" not in result.stdout, f"{label}: premature game-over result"
    return captures


def main():
    invalid = [
        "", "hello", "0 0 0", "0 0 0 1 2", "0+0+0+1", "0 0 0 1junk",
        "0.0 0 0 1", "-1 0 0 0", "5 0 4 0", "0 6 0 5", "0 0 0 0",
        "0 0 1 1", "0 0 0 2", "9" * 80 + " 0 0 0",
        "0 0 0 1" + " " * 200,
    ]
    fixtures = ["a_wins_game.txt", "b_wins_game.txt", "tie_game.txt"]
    for filename in fixtures:
        moves = (ROOT / "tests" / "inputs" / filename).read_text().splitlines()
        assert moves.pop(0) == "1", f"{filename}: missing two-player menu choice"
        check_game(filename, moves, invalid, trailing_newline=False)
        print(f"PASS {filename}: every board, score, turn, and result")

    captures = {0: 0, 1: 0, 2: 0}
    for seed in range(100):
        rng = random.Random(seed)
        shuffled = MOVES.copy()
        rng.shuffle(shuffled)
        moves = []
        for move in shuffled:
            if rng.choice([True, False]):
                move = (*move[2:], *move[:2])
            moves.append(rng.choice([" ", "  ", "\t"]).join(map(str, move)))
        counts = check_game(f"seed {seed}", moves, invalid, trailing_newline=bool(seed % 2))
        for count in captures:
            captures[count] += counts[count]
    assert all(captures.values()), "random games did not cover all capture counts"
    print(f"PASS 100 seeded full games (4,900 legal moves): capture counts {captures}")
    print("PASS malformed, oversized, out-of-range, diagonal, repeated, and reversed input recovery")

    for length in (0, 1, 10, 48):
        moves = [" ".join(map(str, move)) for move in MOVES[:length]]
        check_game(f"EOF after {length} moves", moves, trailing_newline=False)
    print("PASS EOF before play and after 1, 10, and 48 moves, including missing final newline")
    print("All 107 end-to-end game scenarios passed.")


if __name__ == "__main__":
    main()
