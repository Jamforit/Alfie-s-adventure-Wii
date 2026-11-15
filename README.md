# Alfie's Adventure v0.2 — Nintendo Wii

A custom family-friendly Wii homebrew platformer starring **the real Alfie**, based on his photographs. Made for **Kacey & Darcey**.

## New in v0.2

- Custom playable Alfie character: black puppy, floppy ears, white chest patch. Four art poses (idle, running, jumping and super-powered), derived from Alfie-inspired concept art.
- **Three colourful levels:** The Garden, The Big Park and The Sunny Beach.
- **Bark at enemies** (cats, squirrels and crabs). Jump on them or use a poo boost to defeat them too.
- **Poo power-ups** give a silly speed-and-jump boost for a few seconds. Yes, Alfie eats them. 🤣
- Nine bones per level, with six needed to get home. Tennis-ball bonuses, three hearts, score and a kid-friendly retry screen.
- Original synthesized sound effects for jump, treat, ball, poo, bark, hurt, enemy and victory. Music is not included in this version.
- Special title and winning screens for Kacey and Darcey.

## Wii Remote (held sideways)

| Button | Action |
|---|---|
| D-pad | Run left/right |
| 2 | Jump, start, next level, retry |
| 1 | Bark (repels nearby enemies) |
| + | Restart current level |
| HOME | Exit to Homebrew Channel |

## Install from iPhone with FTP

1. Go to **Actions → latest successful build → Artifacts → alfies-adventure-wii** (or use the attached release ZIP).
2. Extract ZIP in iPhone **Files** app; you'll get `apps/alfies_adventure/boot.dol` and `meta.xml`.
3. Start WiiXplorer's FTP server on the Wii.
4. In your iPhone FTP manager, open SD card `/apps/alfies_adventure/` and replace the *previous game* `boot.dol` and `meta.xml` with these new ones. Don't touch system folders or other homebrew.
5. Quit WiiXplorer, reopen the Homebrew Channel, launch Alfie's Adventure.

## Source and build

- C source and Wii GX renderer: `source/main.c`
- Independent physics and gameplay logic: `source/game.h`
- Embedded textured character art: `source/sprites.h`
- Host regression tests: `tests/test_game.c` (`make test`)
- `make` in a devkitPro devkitPPC / libogc environment produces `boot.dol`.
- GitHub Actions uses the official `devkitpro/devkitppc` image.

The Wii build is a homebrew application only. It does not patch IOS, install WADs, or change Wii system software.
