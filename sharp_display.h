#ifndef SHARP_DISPLAY_H
#define SHARP_DISPLAY_H

#include "hardware/spi.h"
#include <stdint.h>

#define SHARP_WIDTH 400
#define SHARP_HEIGHT 240
#define SHARP_WIDTH_BYTES (SHARP_WIDTH / 8)
#define SHARP_BUF_SIZE (SHARP_WIDTH_BYTES * SHARP_HEIGHT)

typedef struct {
    uint8_t framebuffer[SHARP_BUF_SIZE];
    spi_inst_t *spi;
    uint8_t cs_pin;
    uint8_t vcom;
} SharpDisplay;

void sharp_display_init(SharpDisplay *d, spi_inst_t *spi, uint8_t cs_pin,
                        uint8_t sck_pin, uint8_t mosi_pin, uint32_t freq_hz);
void sharp_display_clear(SharpDisplay *d);
void sharp_display_refresh(SharpDisplay *d);

#endif
