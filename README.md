# NX-trex

A Vectrex emulator for the Nintendo Switch.

NX-trex is a port of [VecX](http://vecx.malban.de/), the classic Vectrex
emulator by Valavan Manohararajah, wrapped in a native Switch frontend
with a custom menu, SD card cart loading, and analog stick input.

## Legal

NX-trex includes the Vectrex BIOS (`rom.dat`) to ensure the emulator runs
on real hardware. On October 27, 1992, Smith Engineering granted the
community permission to freely distribute all Vectrex-related materials
(BIOS, games, manuals, documentation) for non-commercial purposes.
See [`LEGAL.txt`](LEGAL.txt) for full details.

<p align="center">
  <img src="images/Captura%20de%20pantalla%202026-09-16%20023353.png" alt="NX-trex main menu" width="48%" />
  <img src="images/Captura%20de%20pantalla%202026-09-16%20023415.png" alt="NX-trex cart browser" width="48%" />
</p>




## Features

- Runs commercial Vectrex cartridges (`.vec` , 4K-8K)
- Built-in BIOS with Mine Storm
- Native Switch menu with cart browser
- Analog stick input with response curve for precision games
- SD card cart loading
- Sound via AY-3-8912 emulation

## Requirements

- A Nintendo Switch with homebrew enabled (Atmosphère or similar)
- Optionally, Vectrex cartridge dumps in `.vec`  format

## Installation

1. Copy `NXtrex.nro` to `sdmc:/switch/` on your SD card.
2. (Optional) Create `sdmc:/NX-trex/roms/` and drop `.vec` files there.
3. Launch NX-trex from the homebrew menu.

The emulator will create `sdmc:/NX-trex/` and `sdmc:/NX-trex/roms/`
automatically if they don't exist.

## Usage

On startup you'll see the main menu:

- **RUN MINE STORM** - boot the built-in game from the BIOS
- **LOAD CART** - browse and load `.vec` files from `sdmc:/NX-trex/roms/`
- **EXIT** - quit to the homebrew menu

### Controls

| Switch            | Vectrex         |
|-------------------|-----------------|
| Left analog stick | Analog joystick |
| A / B / X / Y     | 1 / 2 / 3 / 4   |
| Minus             | Return to menu  |
| Plus              | Quit            |

## Building

Requires devkitPro with `switch-dev`.

## Changes from upstream VecX

NX-trex is not a bit-for-bit copy of VecX. Beyond the Switch frontend,
the emulator core itself has been modified in a few places.

### CPU (e6809)

- Fixed an uninitialized read path in the 6809 memory decoder that
  could return garbage for certain unhandled address ranges.
- Consolidated two divergent forks of the `e6809.c` core into a
  single source file with a merged interface.
- Corrected the return-value semantics of a small number of
  condition-code helpers on edge inputs.
- Signature and interface changes to integrate cleanly with the
  host's function-pointer I/O model.

### Sound (e8910)

- Resampled the AY-3-8912 output to the Switch's native audio rate
  so it plays through the standard libnx audio path without an
  external mixer library.
- Added a per-frame `e8910_update()` hook so the audio buffer
  drains in step with the emulator loop rather than free-running.
- Register writes are still bit-identical to upstream; only the
  output stage is different.

### Everything else

The 6522 VIA, analog integrator, vector generator, and cartridge
mapper are unchanged from VecX and behave identically.


### TODO

- Better sound emulation
- 2-player mode
- Test with more carts, especially edge cases (unusual sizes, non-standard headers)

## Credits

- **VecX** by Valavan Manohararajah, the CPU, VIA, analog, and sound core
- **font8x8_basic** by Daniel Hepper (public domain), menu font
- **libnx** / **devkitPro**, Switch toolchain and runtime
- Vectrex BIOS and games © Smith Engineering / Jay Smith

NX-trex is not affiliated with or endorsed by Nintendo, Smith
Engineering, or Jay Smith.