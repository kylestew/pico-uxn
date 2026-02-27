#include "sharp_display.h"
#include "pico/stdlib.h"
#include <string.h>

#define SHARP_BIT_WRITECMD 0x80
#define SHARP_BIT_VCOM 0x40
#define SHARP_BIT_CLEAR 0x20

static uint8_t reverse_bits(uint8_t b) {
    uint8_t r = 0;
    for (uint8_t i = 0; i < 8; i++) {
        r = (r << 1) | (b & 1);
        b >>= 1;
    }
    return r;
}

void sharp_display_init(SharpDisplay *d, spi_inst_t *spi, uint8_t cs_pin,
                        uint8_t sck_pin, uint8_t mosi_pin, uint32_t freq_hz) {
    d->spi = spi;
    d->cs_pin = cs_pin;
    d->vcom = SHARP_BIT_VCOM;
    memset(d->framebuffer, 0, SHARP_BUF_SIZE);

    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 0);

    spi_init(spi, freq_hz);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
}

void sharp_display_clear(SharpDisplay *d) {
    uint8_t cmd[2] = {SHARP_BIT_CLEAR | d->vcom, 0x00};

    gpio_put(d->cs_pin, 1);
    busy_wait_us_32(6);
    spi_write_blocking(d->spi, cmd, 2);
    gpio_put(d->cs_pin, 0);
    busy_wait_us_32(2);

    d->vcom ^= SHARP_BIT_VCOM;
    memset(d->framebuffer, 0, SHARP_BUF_SIZE);
}

void sharp_display_refresh(SharpDisplay *d) {
    uint8_t byte;

    gpio_put(d->cs_pin, 1);
    busy_wait_us_32(6);

    byte = SHARP_BIT_WRITECMD | d->vcom;
    spi_write_blocking(d->spi, &byte, 1);

    for (uint16_t y = 0; y < SHARP_HEIGHT; y++) {
        byte = reverse_bits(y + 1);
        spi_write_blocking(d->spi, &byte, 1);
        spi_write_blocking(d->spi, &d->framebuffer[y * SHARP_WIDTH_BYTES],
                           SHARP_WIDTH_BYTES);
        byte = 0x00;
        spi_write_blocking(d->spi, &byte, 1);
    }

    byte = 0x00;
    spi_write_blocking(d->spi, &byte, 1);

    gpio_put(d->cs_pin, 0);
    busy_wait_us_32(2);

    d->vcom ^= SHARP_BIT_VCOM;
}
