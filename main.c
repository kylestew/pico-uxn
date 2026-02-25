#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

int main() {
    bi_decl(bi_program_description("Uxn on the Raspberry Pi Pico"));

    stdio_init_all();

    while (1) {
        puts("Hello from pico-uxn");
        sleep_ms(1000);
    }
}
