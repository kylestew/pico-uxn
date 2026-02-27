#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "rom.h"
#include "sharp_display.h"
#include "uxn.h"

#include <stdio.h>
#include <string.h>

static SharpDisplay display;

/*
@|System ------------------------------------------------------------ */

static unsigned int
system_load() {
    memcpy(ram + 0x100, test_rom, test_rom_len);
    return 1;
}

static unsigned int
system_boot() {
    return system_load();
}

// static void fill_checkerboard(SharpDisplay *d, int square_size) {
//     for (int y = 0; y < SHARP_HEIGHT; y++) {
//         int checker_y = y / square_size;
//         for (int x_byte = 0; x_byte < SHARP_WIDTH_BYTES; x_byte++) {
//             int checker_x                                  = (x_byte * 8) / square_size;
//             d->framebuffer[y * SHARP_WIDTH_BYTES + x_byte] = ((checker_x + checker_y) % 2) ? 0xFF : 0x00;
//         }
//     }
// }


/*
@|Devices ----------------------------------------------------------- */

Uint8 emu_dei(Uint8 port) {
    return dev[port];
}

void emu_deo(Uint8 port, Uint8 value) {
    dev[port] = value;
}

int main() {
    stdio_init_all();

    // == DISPLAY ===
    // Set SPI0 at 2MHz
    sharp_display_init(&display, spi_default, PICO_DEFAULT_SPI_CSN_PIN, PICO_DEFAULT_SPI_SCK_PIN,
                       PICO_DEFAULT_SPI_TX_PIN, 2000000);
    sleep_ms(1000);
    sharp_display_clear(&display);

    // == SYSTEM BOOT ==
    system_boot();

    // == EXEC ==
    uxn_eval(0x100);

    // == HALT ==
    while (1) {
    }
}
