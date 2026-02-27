# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Uxn virtual machine implementation running on a Raspberry Pi Pico 2 W (RP2350/Cortex-M33). The Uxn bytecode ROM is baked into the firmware at compile time. Output is rendered to a Sharp Memory LCD display over SPI.

## Build Commands

Requires: `arm-none-eabi-gcc` toolchain, `cmake`, `xxd`.

```sh
# Configure (first time, or after adding source files)
cmake -B build -DPICO_BOARD=pico2_w

# Build
make -C build -j4

# Flash: hold BOOTSEL on Pico, then copy UF2 to the mounted drive
cp build/pico_uxn.uf2 /Volumes/RPI-RP2/
```

The build will print memory usage via `arm-none-eabi-size` after each successful compile.

## Architecture

### ROM Loading

`test.rom` (a Uxn binary) is converted at build time to `build/rom.h` by `xxd -i`. The ROM contents are copied into `ram[0x100]` at boot via `system_load()` in `main.c`. Uxn programs always start executing at address `0x100`.

### Uxn VM (`uxn.c` / `uxn.h`)

The VM state is in global arrays:

- `ram[0x10000]` — 64 KB address space
- `dev[0x100]` — device page (256 bytes), accessed via DEI/DEO instructions
- `stk[2][0x100]` — working stack (`stk[0]`) and return stack (`stk[1]`)
- `ptr[2]` — stack pointers for each stack

The entire opcode dispatch is implemented as a `switch` over macros using the `OPC()` macro, which generates 8 case variants per opcode to handle mode bits: short (`d=1`), return (`r=1`), and keep (`k`).

Device I/O is decoupled via two callbacks implemented in `main.c`:

- `emu_dei(port)` — called when the VM reads a device port (DEI opcode)
- `emu_deo(port, value)` — called when the VM writes a device port (DEO opcode)

Currently these just pass through to `dev[]`. Adding peripheral support means adding `switch` cases in `emu_deo`/`emu_dei`.

### Sharp Display (`sharp_display.c` / `sharp_display.h`)

400×240 monochrome SPI display (1 bit/pixel = 12,000 byte framebuffer). The `SharpDisplay` struct holds the framebuffer and SPI config. The VCOM bit must be toggled every refresh to prevent DC bias damage to the display — this is handled automatically in `sharp_display_clear()` and `sharp_display_refresh()`. Row addresses are sent MSB-first (bit-reversed), so `reverse_bits()` is applied to line numbers.

### Reference Implementation

`docs/uxnmin.c` is a standalone desktop Uxn emulator (single-file, stdio-based) useful for testing ROM logic without hardware. It includes console device support (ports `0x18`/`0x19` for stdout/stderr).

`docs/uxn2.c` is a full implementation using SDL2. Our implementation won't be using SDL,
but instead our custom Sharp display driver. We also don't have a file system. Much of the
rest of the code is helpful as reference.

## Key Files

| File                                  | Purpose                                                    |
| ------------------------------------- | ---------------------------------------------------------- |
| `main.c`                              | Entry point, system boot, `emu_dei`/`emu_deo` device hooks |
| `uxn.c` / `uxn.h`                     | Uxn bytecode interpreter and VM state                      |
| `sharp_display.c` / `sharp_display.h` | Sharp Memory LCD SPI driver                                |
| `test.rom`                            | Uxn ROM binary (compiled to `build/rom.h` at build time)   |
| `docs/uxnmin.c`                       | Reference desktop Uxn emulator for testing                 |
| `pico-sdk/`                           | Raspberry Pi Pico SDK (git submodule)                      |

## Uxn Device Map Convention

Uxn devices are mapped to `dev[]` in 16-byte pages. The standard Uxn device layout (ports `0x00`–`0xff`) should guide any device emulation additions. Currently no devices beyond raw `dev[]` passthrough are implemented.
