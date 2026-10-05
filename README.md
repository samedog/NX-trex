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
- 2-player mode (one sideways Joy-Con each; menu driven by Player 1)
- SD card cart loading
- Sound via AY-3-8912 emulation

## Requirements

- A Nintendo Switch with homebrew enabled (Atmosphère or similar)
- Optionally, Vectrex cartridge dumps in `.vec`  format

## Installation

1. Copy `NX-trex.nro` to `sdmc:/switch/` on your SD card.
2. (Optional) Create `sdmc:/NX-trex/roms/` and drop `.vec` files there.
3. Launch NX-trex from the homebrew menu.

The emulator will create `sdmc:/NX-trex/` and `sdmc:/NX-trex/roms/`
automatically if they don't exist.

## Usage

On startup you'll see the main menu:

- **RUN MINE STORM** - boot the built-in game from the BIOS
- **LOAD CART** - browse and load `.vec` files from `sdmc:/NX-trex/roms/`
- **2P MODE: ON / OFF** - toggle 2-player mode
- **EXIT** - quit to the homebrew menu

### Controls

1 Player mode:

| Switch            | Vectrex         |
|-------------------|-----------------|
| Left analog stick | Analog joystick |
| B / A / Y / X     | 1 / 2 / 3 / 4   |
| L (or Minus)      | Return to menu  |
| R (or Plus)       | Quit            |


2 Player mode (each player holds one Joy-Con sideways: Joy-Con L is rotated -90°, Joy-Con R is rotated +90°):

| Joycon L (Player 1)                              | Vectrex         |
|--------------------------------------------------|-----------------|
| Left analog stick                                | Analog joystick |
| left(down) / down(right) / up(left) / right(up)  | 1 / 2 / 3 / 4   |
| SL (or Minus)                                    | Return to menu  |
| SR (or Plus)                                     | Quit            |

| Joycon R (Player 2)       | Vectrex         |
|---------------------------|-----------------|
| Right analog stick        | Analog joystick |
| A / X / B / Y             | 1 / 2 / 3 / 4   |

### Menu

The menu is always driven by **Player 1**. In 2P mode Player 1 is the
**Left Joy-Con** (held sideways); the Right Joy-Con (Player 2) is not used
by the menu. Navigation is with the analog stick only.

| Action          | 1P mode      | 2P mode (Player 1 = Left Joy-Con) |
|-----------------|--------------|-----------------------------------|
| Move            | Left stick   | Left stick (rotated)              |
| Confirm (btn 4) | X            | D-Pad Right                       |
| Back (btn 1)    | B            | D-Pad Left                        |
| Return to menu  | L (or Minus) | SL (or Minus)                     |
| Quit            | R (or Plus)  | SR (or Plus)                      |

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

- The AY-3-8912 core is unchanged; only the output stage differs.
- The mixer output (`0..4095`, the volume domain) is scaled by 8 to fill
  signed 16-bit range, then fed to the standard libnx `audout` path at its
  native 48 kHz (mono duplicated to stereo), with no external mixer.
- `e8910_update()` drains every free buffer each 30 fps frame. The device
  consumes about 1.5 buffers per frame, so refilling only one starved the
  queue; draining all released buffers keeps it fed.

### Everything else

The 6522 VIA, analog integrator, vector generator, and cartridge
mapper are unchanged from VecX and behave identically.


### TODO

- Verify sound against more carts (Armor Attack and similar)
- Centre the audio output to remove the DC offset / note-onset transients
- Test with more carts, especially edge cases (unusual sizes, non-standard headers)

## Credits

- **VecX** by Valavan Manohararajah, the CPU, VIA, analog, and sound core
- **font8x8_basic** by Daniel Hepper (public domain), menu font
- **libnx** / **devkitPro**, Switch toolchain and runtime
- Vectrex BIOS and games © Smith Engineering / Jay Smith

NX-trex is not affiliated with or endorsed by Nintendo, Smith
Engineering, or Jay Smith.