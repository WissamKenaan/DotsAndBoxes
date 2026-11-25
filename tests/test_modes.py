"""Exercise the real menu and human-vs-bot loop with an independent model."""

import queue
import random
import re
import subprocess
import threading

from test_game import BOXES, GAME, HEADER, MOVES, ROOT, edge, read_board


def check_menu():
    tie = (ROOT / "tests/inputs/tie_game.txt").read_text().splitlines()[1:]
    scenarios = [
        ("menu EOF", "", 0, 0, "Input closed at the menu. Exiting."),
        ("invalid choices", "\nhello\n0\n3\n1 2\n2junk\n" + "9" * 200 + "\n", 0, 7,
         "Input closed at the menu. Exiting."),
        ("two players EOF", "1\n", 1, 0, "Input closed before the game finished."),
        ("bot mode EOF", "2\n", 1, 0, "Input closed before the game finished."),
        ("choice without newline", "2", 1, 0, "Input closed before the game finished."),
        ("whitespace choice", " \t1 \t\n" + "\n".join(tie), 0, 0, "It's a tie!"),
        ("long choice recovery", "1" + " " * 200 + "\n1\n" + "\n".join(tie), 0, 1,
         "It's a tie!"),
    ]
    for label, payload, status, invalid, expected in scenarios:
        result = subprocess.run([str(GAME)], input=payload, text=True,
                                capture_output=True, timeout=10, cwd=ROOT)
        assert result.returncode == status, (label, result.returncode)
        assert not result.stderr, (label, result.stderr)
        assert result.stdout.count("Invalid choice.") == invalid, label
        assert expected in result.stdout, label
        assert "Bot (Player B) plays:" not in result.stdout, label
        if "menu EOF" == label or "invalid choices" == label:
            assert HEADER not in result.stdout, label
        print(f"PASS {label}")

    result = subprocess.run([str(GAME)], input=(ROOT / "tests/inputs/bot_game.txt").read_text(),
                            text=True, capture_output=True, timeout=10, cwd=ROOT)
    assert result.returncode == 0 and not result.stderr
    assert "Bot (Player B) plays:" in result.stdout
    assert "Game over! Final score:" in result.stdout
    assert "Player B's turn." not in result.stdout
    print("PASS repeatable bot-mode fixture")


def check_bot_game(seed):
    rng = random.Random(seed)
    process = subprocess.Popen([str(GAME)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, bufsize=1, cwd=ROOT)
    output = queue.Queue()

    def read_output():
        try:
            for line in process.stdout:
                output.put(line.rstrip("\r\n"))
        finally:
            output.put(None)

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()
    drawn, owners = set(), {}
    player = "A"
    boards = scores = moves = 0
    captures = {"A": 0, "B": 0, "double": 0}
    sent_invalid = False
    score_a = None
    result_lines = []

    def next_line():
        try:
            return output.get(timeout=10)
        except queue.Empty:
            raise AssertionError(f"seed {seed}: game stopped responding") from None

    def apply_move(move, actor):
        nonlocal player, moves
        assert actor == player, f"seed {seed}: wrong player's move"
        assert move in MOVES or (*move[2:], *move[:2]) in MOVES, "non-neighbour bot move"
        line = edge(move[:2], move[2:])
        assert line not in drawn, "bot repeated a drawn edge"
        drawn.add(line)
        completed = [box for box, sides in BOXES.items() if box not in owners and sides <= drawn]
        for box in completed:
            owners[box] = actor
        if completed:
            captures[actor] += 1
            captures["double"] += len(completed) == 2
        else:
            player = "B" if player == "A" else "A"
        moves += 1

    try:
        process.stdin.write("2\n")
        process.stdin.flush()
        while True:
            line = next_line()
            if line is None:
                break
            if line == HEADER:
                picture = [next_line() for _ in range(9)]
                assert all(part is not None for part in picture), "incomplete picture"
                assert read_board(picture) == (drawn, owners), f"seed {seed}: wrong board at {moves}"
                boards += 1
            elif line.startswith("Player A's turn."):
                assert player == "A" and len(drawn) < 49, "incorrect human turn"
                if not sent_invalid:
                    process.stdin.write("hello\n")
                    process.stdin.flush()
                    sent_invalid = True
                    continue
                available = [move for move in MOVES if edge(move[:2], move[2:]) not in drawn]
                # Half the games use a human who captures whenever possible;
                # the other half choose uniformly from the remaining edges.
                if seed % 2:
                    taking = [move for move in available if any(
                        box not in owners and sides <= drawn | {edge(move[:2], move[2:])}
                        for box, sides in BOXES.items())]
                    available = taking or available
                move = rng.choice(available)
                apply_move(move, "A")
                process.stdin.write(" ".join(map(str, move)) + "\n")
                process.stdin.flush()
            elif line.startswith("Bot (Player B) plays:"):
                match = re.fullmatch(r"Bot \(Player B\) plays: (\d+) (\d+) (\d+) (\d+)", line)
                assert match, "unreadable bot announcement"
                apply_move(tuple(map(int, match.groups())), "B")
            elif line.startswith("Player B's turn."):
                raise AssertionError("bot mode requested keyboard input for B")
            elif line.startswith("Player A score:"):
                score_a = int(line.split(":")[1])
            elif line.startswith("Player B score:"):
                actual = (score_a, int(line.split(":")[1]))
                expected = (list(owners.values()).count("A"), list(owners.values()).count("B"))
                assert actual == expected, f"seed {seed}: wrong score after move {moves}"
                scores += 1
            elif line.startswith("Game over!") or line.endswith("wins!") or line == "It's a tie!":
                result_lines.append(line)
            elif "Input closed" in line:
                raise AssertionError("unexpected EOF during interactive game")
            elif line.startswith("Invalid move:"):
                assert moves == 0 and sent_invalid, "legal move rejected"
        assert process.wait(timeout=5) == 0
        assert (moves, boards, scores, len(owners)) == (49, 50, 49, 20)
        a, b = list(owners.values()).count("A"), list(owners.values()).count("B")
        winner = "It's a tie!" if a == b else f"Player {'A' if a > b else 'B'} wins!"
        assert result_lines == [f"Game over! Final score: A = {a}, B = {b}", winner]
        return captures
    finally:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=5)
        process.stdin.close()
        reader.join(timeout=5)
        process.stdout.close()


def main():
    check_menu()
    captures = {"A": 0, "B": 0, "double": 0}
    for seed in range(30):
        counts = check_bot_game(seed)
        for key in captures:
            captures[key] += counts[key]
    assert all(captures.values()), "missing human/bot extra-turn or double-capture coverage"
    print(f"PASS 30 interactive human-vs-bot games (1,470 legal moves): {captures}")
    print("PASS every board, ownership, score, bot move, extra turn, and final result")
    print("All 38 menu and bot-mode scenarios passed.")


if __name__ == "__main__":
    main()
