#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "sharp_display.h"

#include <stdio.h>

static SharpDisplay display;

static void fill_checkerboard(SharpDisplay *d, int square_size) {
    for (int y = 0; y < SHARP_HEIGHT; y++) {
        int checker_y = y / square_size;
        for (int x_byte = 0; x_byte < SHARP_WIDTH_BYTES; x_byte++) {
            int checker_x                                  = (x_byte * 8) / square_size;
            d->framebuffer[y * SHARP_WIDTH_BYTES + x_byte] = ((checker_x + checker_y) % 2) ? 0xFF : 0x00;
        }
    }
}

int main() {
    bi_decl(bi_program_description("Uxn on the Raspberry Pi Pico"));

    stdio_init_all();

    // Set SPI0 at 2MHz
    sharp_display_init(&display, spi_default, PICO_DEFAULT_SPI_CSN_PIN, PICO_DEFAULT_SPI_SCK_PIN,
                       PICO_DEFAULT_SPI_TX_PIN, 2000000);

    sleep_ms(1000);

    sharp_display_clear(&display);

    // fill_checkerboard(&display, 24);
    // sharp_display_refresh(&display);

    bool toggle = false;
    while (1) {
        puts("Hello from pico-uxn");
        sleep_ms(1000);

        if (toggle) {
            fill_checkerboard(&display, 24);
            sharp_display_refresh(&display);
        } else {
            sharp_display_clear(&display);
        }
        toggle = !toggle;
    }
}
