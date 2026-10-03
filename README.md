# M5 Pokedex

A pocket Pokedex for the [M5Stack CoreInk](https://docs.m5stack.com/en/core/coreink): all 386 Pokemon from Generations I to III on a 1.54" e-ink screen, fully offline, running on battery.

Scroll the list with the side dial, open any Pokemon to see its sprite, stats and Pokedex entry, or hold the top button and let it pick one for you. When you put it down it turns itself off, and the e-ink screen keeps showing the last Pokemon you looked at.

## Features

- **386 Pokemon** (#001 Bulbasaur to #386 Deoxys), stored on the device; no WiFi needed
- **Three pages per Pokemon**: a large sprite with its types, base stats with bars and height/weight, and the Pokedex description
- **Game Boy-style sprites**: each sprite is converted to black, white and a checker shade with a clean outline, so it reads well on e-ink
- **Jump to any number** with the dial, or **pick one at random**
- **Battery friendly**: powers off after a minute of inactivity and remembers where you were
- **Gentle on the eyes**: moving through the list only redraws the rows that changed, and the full-screen "flash" that clears e-ink ghosting only happens occasionally

## Controls

The CoreInk has a three-way dial on the side (up, down, press) and a button on top.

| Where | Dial up / down | Dial press | Top button tap | Top button hold |
|---|---|---|---|---|
| List | Move the cursor (hold to skip 10) | Open the Pokemon | Jump to a number | Random Pokemon |
| Pokemon | Previous / next Pokemon | Next page | Back to the list | Random Pokemon |
| Jump | Change the digit | Confirm the digit | Cancel | Random Pokemon |

The power button wakes the device and you continue where you left off. While it's on, a tap on the power button cleans up any leftover ghosting on the screen.

## What you need

- An M5Stack CoreInk (ESP32-PICO-D4, 4 MB flash, 200x200 e-ink)
- A USB-C cable
- [PlatformIO](https://platformio.org/) (`pipx install platformio` or `uv tool install platformio`)

On Linux your user needs access to the serial port, usually by joining the `dialout` group (log out and back in afterwards):

```sh
sudo usermod -aG dialout $USER
```

## Build and flash

```sh
git clone https://github.com/pinzon/m5-pokedex.git
cd m5-pokedex
uv run tools/build_dex.py   # generate the Pokedex data (see below)
pio run -t upload
```

The upload port is set to `/dev/ttyACM0` in `platformio.ini`; change `upload_port` and `monitor_port` if your board shows up elsewhere. `pio device monitor` shows a boot message and every button press, which helps when something doesn't respond as expected.

## Generating the Pokedex data

All Pokemon data lives in a single file, `data/dex.bin`, which is built into the firmware. It isn't included in this repository because it contains Pokemon artwork and text, so you generate it yourself from [PokeAPI](https://pokeapi.co/) with a Python script (needs [uv](https://docs.astral.sh/uv/)):

```sh
uv run tools/build_dex.py
```

The build fails with a missing-file error until this has been run once. The first run downloads about 1,200 files (a few minutes) and caches them in `tools/cache/`, so later runs work offline. It also writes a few sample sprites to `tools/preview/` so you can check how the conversion looks. Change `COUNT` in the script to include more generations (the full National Dex still fits in flash).

## How it works

- **Data**: each Pokemon is a fixed-size 3 KB record (name, category, types, height, weight, base stats, description and a 144x144 one-bit sprite). The firmware reads the records straight from flash, so the 1.2 MB of data uses almost no RAM.
- **Sprites**: the script crops each sprite, scales it up with crisp pixel edges, then maps dark tones to black, mid tones to a checkerboard and light tones to white, and adds a black outline. Regular dithering turned out noisy and made pale Pokemon like Lugia nearly invisible.
- **Screen updates**: every button press does a fast refresh of only the part of the screen that changed. Fast refreshes slowly leave faint "ghosts" of earlier images, so after 5 refreshes and 5 seconds without input the screen does one slower, flashing redraw to clean up. It does the same right before powering off, and you can trigger it any time with a tap on the power button.
- **Saving your place**: the current screen, Pokemon and page are saved to flash shortly after you stop pressing buttons, and restored when the device wakes up.

## Project layout

```
src/main.cpp        screens, navigation, idle timers, power-off
src/ui.cpp          drawing the list, jump box and Pokemon pages
src/input.cpp       dial and button handling (hold, repeat, tap vs. hold)
src/dex.cpp         reading Pokemon records from the embedded data file
tools/build_dex.py  builds data/dex.bin from PokeAPI
data/dex.bin        the generated Pokedex data (not in git)
```

Built with [M5Unified](https://github.com/m5stack/M5Unified) and [M5GFX](https://github.com/m5stack/M5GFX) on the Arduino framework.

## Credits

Pokemon data and sprites come from [PokeAPI](https://pokeapi.co/). Pokemon and all related names and artwork are trademarks and copyright of Nintendo, Game Freak and The Pokemon Company. This is an unofficial fan project and is not affiliated with or endorsed by them.
