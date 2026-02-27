# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Uxn virtual machine implementation running on a Raspberry Pi Pico 2 W (RP2350/Cortex-M33). The Uxn bytecode ROM is baked into the firmware at compile time. Output is rendered to a Sharp Memory LCD display over SPI.

## Build Commands

Requires: `arm-none-eabi-gcc` toolchain, `cmake`, `xxd`, SDL2 (for host assembler tool).

```sh
# Build with default test (opctest)
make

# Build with a specific test
make TEST_TAL=screen

# Flash via Debug Probe
make flash
```

The build assembles `etc/tests/${TEST_TAL}.tal` into a ROM at build time using a self-hosted toolchain (see ROM Assembly below), then bakes it into the firmware. Memory usage is printed after each successful compile.

## Architecture

### ROM Assembly (Self-Hosted)

The ROM is assembled from `.tal` source at build time via a self-hosted pipeline:

1. `etc/utils/uxn2.c` is compiled as a host-native binary (using `cc`, not the ARM cross-compiler)
2. `etc/utils/drifblim.rom.txt` is converted from hex to binary via `xxd -r -p`
3. The selected `.tal` file is assembled: `uxn2 drifblim.rom input.tal output.rom`
4. The output ROM is converted to `build/rom.h` by `xxd -i`

The ROM contents are copied into `ram[0x100]` at boot via `system_load()` in `main.c`. Uxn programs always start executing at address `0x100`. Select the test with `-DTEST_TAL=name` (CMake) or `make TEST_TAL=name`.

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

`etc/utils/uxn2.c` is the full Uxn emulator with SDL2 and file device support. It is compiled as a host tool at build time to run the drifblim assembler. It is not linked into the Pico firmware.

## Key Files

| File                                  | Purpose                                                    |
| ------------------------------------- | ---------------------------------------------------------- |
| `main.c`                              | Entry point, system boot, `emu_dei`/`emu_deo` device hooks |
| `uxn.c` / `uxn.h`                     | Uxn bytecode interpreter and VM state                      |
| `sharp_display.c` / `sharp_display.h` | Sharp Memory LCD SPI driver                                |
| `etc/tests/*.tal`                     | Uxn test programs in Tal assembly                          |
| `etc/utils/uxn2.c`                    | Full Uxn emulator (host build tool for assembler)          |
| `etc/utils/drifblim.rom.txt`          | Drifblim assembler ROM (hex dump)                          |
| `docs/uxnmin.c`                       | Minimal reference Uxn emulator (console I/O only)          |
| `pico-sdk/`                           | Raspberry Pi Pico SDK (git submodule)                      |

## Uxn Device Map Convention

Uxn devices are mapped to `dev[]` in 16-byte pages. The standard Uxn device layout (ports `0x00`–`0xff`) should guide any device emulation additions. Currently no devices beyond raw `dev[]` passthrough are implemented.
