# GitHub Workflow

## One-time setup

```sh
git config --global user.name  "Your Name"
git config --global user.email "you@mail.aub.edu"
```

Create an **empty** repository on github.com named `DotsAndBoxes` (no README,
no .gitignore; the project already has both). GitHub requires a
**Personal Access Token** instead of your password for `git push` over HTTPS:
GitHub → Settings → Developer settings → Personal access tokens.

## Commit in phases (meaningful history)

The grading looks for regular, meaningful commits, so don't upload everything
in one commit. Run these from the project folder, one phase at a time:

```sh
git init
git branch -M main

git add .gitignore README.md Makefile
git commit -m "Add project skeleton: Makefile, README, .gitignore"

git add include/board.h src/board.c
git commit -m "Add board model with move validation and O(1) box completion"

git add include/display.h src/display.c
git commit -m "Add console display matching the sample board format"

git add include/input.h src/input.c
git commit -m "Add safe move input parsing with fgets and strtol"

git add include/game.h src/game.c src/main.c
git commit -m "Add two-player game loop with scoring and extra turn on capture"

git add tests/
git commit -m "Add unit tests and scripted full-game tests (tie, A wins, B wins, invalid input)"

git add scripts/ docs/ALPINE_SETUP.md
git commit -m "Add Alpine VM auto-start configuration"

git add docs/
git commit -m "Add design, testing and GDB/Valgrind debugging documentation"

git remote add origin https://github.com/<your-username>/DotsAndBoxes.git
git push -u origin main
```

## Ongoing work

```sh
git status                 # what changed?
git diff                   # see the changes
git add <files>
git commit -m "Fix: <what and why>"
git push
```

Good message: `Fix: reject moves repeated in reverse order`.
Bad message: `update`, `changes`, `final final`.

Add screenshots from your Alpine VM (GDB, Valgrind, auto-start) to
`docs/screenshots/` and commit them; they are evidence for the debugging part
of the grade.

## Moodle submission

The PDF also asks for a zip named `group-name.DotsAndBoxes.zip`:

```sh
make clean
cd ..
zip -r groupname.DotsAndBoxes.zip DotsAndBoxes -x "DotsAndBoxes/.git/*"
```
