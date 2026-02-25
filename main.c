#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"

int main() {
    bi_decl(bi_program_description("Uxn on the Raspberry Pi Pico"));

    stdio_init_all();

    if (cyw43_arch_init()) {
        puts("CYW43 init failed");
        return 1;
    }

    while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        puts("Hello from pico-uxn");
        sleep_ms(250);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);
    }
}
