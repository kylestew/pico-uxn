# pico-uxn

Uxn on the Raspberry Pi Pico — command-line toolchain with a Debug Probe.

## Prerequisites (macOS)

Install Xcode command-line tools (provides Git, Tar, Clang):

```bash
xcode-select --install
```

Install the ARM cross-compiler, CMake, and build dependencies:

```bash
brew install cmake arm-none-eabi-gcc sdl2 minicom automake autoconf texinfo libtool libusb pkg-config
```

Build and install the [Raspberry Pi fork of OpenOCD](https://github.com/raspberrypi/openocd) (the stock Homebrew version lacks RP2350 support):

```bash
cd /tmp
git clone https://github.com/raspberrypi/openocd.git rpi-openocd
cd rpi-openocd
git submodule update --init
./bootstrap
./configure --disable-werror
make -j4
make install prefix=$HOME/.local
```

> This installs to `~/.local/bin/openocd`. The Makefile points there by default.

## Get the SDK

The [Pico SDK](https://github.com/raspberrypi/pico-sdk) is included as a git submodule. After cloning this repo:

```bash
git submodule update --init --recursive
```

## Build

```bash
make
```

| Command | Description |
|---------|-------------|
| `make` | Configure and build for Pico 2 W |
| `make flash` | Flash via Debug Probe (SWD) |
| `make debug` | Start OpenOCD server for interactive debugging |
| `make serial` | Open UART console via Debug Probe |
| `make info` | Show binary size and largest symbols |
| `make clean` | Remove build directory |

> Override the board with `make BOARD=pico2` or any other supported board name.

## ROM Assembly (Self-Hosting)

Uxn programs are written in [Tal](https://wiki.xxiivv.com/site/uxntal.html) assembly. The build system assembles `.tal` source files into ROM binaries at build time using a self-hosted toolchain — the assembler ([drifblim](https://wiki.xxiivv.com/site/drifblim.html)) is itself a Uxn ROM, run on a host-compiled Uxn emulator.

The pipeline:

1. `etc/utils/uxn2.c` is compiled as a **host-native** binary (not cross-compiled for ARM)
2. `etc/utils/drifblim.rom.txt` (hex dump of the assembler ROM) is converted to binary via `xxd -r -p`
3. The selected `.tal` file is assembled: `uxn2 drifblim.rom input.tal output.rom`
4. The resulting ROM is converted to a C header via `xxd -i` and baked into the firmware

Select a test program with `TEST_TAL`:

```bash
# Build with a specific test (default: opctest)
make TEST_TAL=screen

# Switch to a different test
make clean
make TEST_TAL=console
```

Available tests in `etc/tests/`: `audio`, `console`, `datetime`, `file`, `opctest`, `perifs`, `screen`, `screen.auto`, `screen.bounds`, `screen.pixel`, `system`.

> Requires SDL2 on the host for the uxn2 build tool: `brew install sdl2`

## Debug Probe Wiring

Connect the Debug Probe to your Pico:

| Debug Probe | Pico                    |
| ----------- | ----------------------- |
| "D" port    | SWD (SWCLK, GND, SWDIO) |
| "U" RX      | Pico TX (GP0)           |
| "U" TX      | Pico RX (GP1)           |
| "U" GND     | Pico GND                |

Plug one USB cable from the Debug Probe to your computer, and a second USB cable from the Pico to your computer (for power).

## Flash via SWD

Load a UF2/ELF onto the Pico through the Debug Probe and OpenOCD:

**RP2040 (Pico):**

```bash
sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
  -c "adapter speed 5000" \
  -c "program build/my_app.elf verify reset exit"
```

**RP2350 (Pico 2):**

```bash
sudo openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
  -c "adapter speed 5000" \
  -c "program build/my_app.elf verify reset exit"
```

> No need to hold BOOTSEL — the Debug Probe handles everything over SWD.

## UART Serial Console

The Debug Probe exposes a USB-to-UART bridge. Connect to it with minicom:

```bash
minicom -D /dev/tty.usbmodem1234561 -b 115200
```

> The device path may differ — run `ls /dev/tty.usbmodem*` to find it.

## Interactive Debugging (GDB over SWD)

Build with debug symbols first (`-DCMAKE_BUILD_TYPE=Debug`).

Start an OpenOCD server in one terminal:

**RP2040:**

```bash
sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000"
```

**RP2350:**

```bash
sudo openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000"
```

In a second terminal, attach with lldb (Arm macOS):

```bash
lldb build/my_app.elf
(lldb) gdb-remote localhost:3333
(lldb) continue
```

Or with gdb-multiarch on Linux:

```bash
gdb-multiarch build/my_app.elf
(gdb) target remote localhost:3333
(gdb) monitor reset init
(gdb) continue
```

## Flash via USB (no Debug Probe)

Hold **BOOTSEL** on the Pico while plugging it in via USB. It mounts as a mass-storage device. Copy the UF2:

```bash
cp build/my_app.uf2 /Volumes/RPI-RP2/
```

The Pico reboots and runs the firmware automatically.
