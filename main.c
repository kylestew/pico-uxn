#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#include <stdio.h>

// void write_register() {
//     spi_write_blockgin
//     cs_deselect();
//     sleep_ms(1);
// }

void cs_select() {
    sleep_ms(1);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    sleep_ms(1);
}
void cs_deselect() {
    sleep_ms(1);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
    sleep_ms(1);
}

void clear_screen() {}

int main() {
    bi_decl(bi_program_description("Uxn on the Raspberry Pi Pico"));

    stdio_init_all();

    // Set SPI0 at 1MHz, TODO: Change to 2MHz once working
    spi_init(spi_default, 1 * 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    // Chip select is active-high, init and drive-low
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);

    // TEMP: delay before clear
    sleep_ms(1000);
    sleep_ms(1000);
    sleep_ms(1000);
    sleep_ms(1000);

    // Clear the screen
    // 00100000
    uint8_t data[2];
    data[0] = 0b00100000;
    data[1] = 0b00000000;

    cs_select();
    spi_write_blocking(spi_default, data, 2);
    cs_deselect();

    while (1) {
        puts("Hello from pico-uxn");
        sleep_ms(1000);
    }
}
