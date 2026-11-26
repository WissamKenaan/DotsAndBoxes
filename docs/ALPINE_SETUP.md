# Alpine Linux VM Setup and Auto-Start

Goal: when the VM boots, the game appears immediately, **with no username or
password prompt**.

> Alpine does **not** use systemd (unlike Ubuntu). It uses **OpenRC** for
> services and **BusyBox init** for the login consoles. The consoles are
> defined in **`/etc/inittab`**, which is what we change.

---

## 1. Create the VM and install Alpine

1. Download the **x86_64 "Virtual"** (or "Standard") ISO from
   <https://alpinelinux.org/downloads/>.
2. In VirtualBox (or VMware/UTM): new Linux VM, type "Other Linux (64-bit)",
   512 MB–1 GB RAM, an 8 GB disk. Attach the ISO and start the VM.
3. Log in as `root` (no password on the live ISO) and run:

   ```sh
   setup-alpine
   ```

   Accept the defaults for keyboard, hostname and network (DHCP), set a
   **root password**, and when asked which disk to use choose `sda` with mode
   **`sys`** (a normal installation to disk).
4. When it finishes: `poweroff`, remove the ISO from the VM settings, and
   start the VM again. Log in as `root`.

## 2. Install the tools

```sh
apk update
apk add build-base gdb valgrind git
```

If `apk` reports that a package cannot be found, enable the community
repository: open `/etc/apk/repositories` with `vi`, remove the `#` at the start
of the line ending in `/community`, save, then run `apk update` again.

## 3. Get the code, build, test, install

```sh
cd /root
git clone https://github.com/WissamKenaan/DotsAndBoxes.git
cd DotsAndBoxes
make
make test
make install        # copies the game to /usr/local/bin/dotsboxes
```

## 4. Create the account that will be logged in automatically

Running the game as `root` would work, but a normal user is safer: a player
at the keyboard cannot damage the system.

```sh
adduser -D player              # create the user (BusyBox adduser)
passwd player                  # set ANY password (see note below)
cp /root/DotsAndBoxes/scripts/player_profile /home/player/.profile
touch /home/player/.hushlogin  # hide the "Welcome to Alpine" message
chown player:player /home/player/.profile /home/player/.hushlogin
```

> **Why set a password if nobody types it?** `adduser -D` may leave the
> account locked. BusyBox `login` refuses locked accounts even when called with
> `-f` (it checks for a `!` or `*` password before it honours `-f`). Giving the
> account a password unlocks it. Nobody will be asked to type it.

`.profile` is run by the login shell. On tty1 it clears the screen, starts the
game, waits for Enter at the end, then logs out. Init then logs in again, so a
new game starts.

## 5. Enable auto-login on tty1

Back up `/etc/inittab`, then replace the tty1 getty line:

```sh
cp /etc/inittab /etc/inittab.bak
sed -i 's|^tty1:.*|tty1::respawn:/bin/login -f player|' /etc/inittab
grep tty1 /etc/inittab
```

The `sed` command replaces the whole line that starts with `tty1:`, whatever
its exact contents. It has exactly three `|` separators:
`s | <find> | <replace with> |`.

The `grep` must print exactly:

```
tty1::respawn:/bin/login -f player
```

If it doesn't, **do not reboot.** Open `/etc/inittab` with `vi` and change the
`tty1::` line by hand. Tip: type `/etc/init` and press **Tab** to let the shell
complete the file name, which avoids typos.

What the line means:

| Part              | Meaning                                                       |
|-------------------|---------------------------------------------------------------|
| `tty1`            | run on the first virtual console (what the VM window shows)   |
| `respawn`         | restart it whenever it exits                                  |
| `/bin/login -f player` | log `player` in **without asking for a password** (`-f`)  |

Leave the `tty2` … `tty6` lines alone: they keep normal login prompts so you
can still administer the VM.

## 6. Reboot and verify

```sh
reboot
```

Expected for Sprint 2: the VM boots straight into the menu (`1) Two players`,
`2) Play against the bot`) on tty1, with no login prompt. Select either mode
to see the empty board. After a completed game, press Enter through the
profile and verify the next session shows the menu again. Screenshots of
these checks are in `docs/screenshots/sprint2/` (05 to 07).

## Getting a root shell after this change

- Press **Alt+F2** (in VirtualBox you may need **Right-Ctrl+F2**) to switch to
  tty2 and log in as `root`. **Alt+F1** returns to the game.
- To undo the auto-start: `cp /etc/inittab.bak /etc/inittab` and `reboot`.

## Alternative method (from the Alpine wiki)

The Alpine wiki's "TTY Autologin" page uses `agetty` instead of BusyBox
`login`:

```sh
apk add agetty
# in /etc/inittab, the tty1 line becomes:
tty1::respawn:/sbin/agetty --autologin player --noclear tty1 linux
```

Use this only if the first method does not work on your Alpine version.
Both methods use the same `.profile` file.

## Updating the game later

```sh
cd /root/DotsAndBoxes && git pull && make && make install
```

The next game on tty1 uses the new binary.
