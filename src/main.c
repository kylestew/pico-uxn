#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "rom.h"
#include "sharp_display.h"
#include "uxn.h"

#include <stdio.h>
#include <string.h>

typedef void (*deo_handler)(void);
typedef Uint8 (*dei_handler)(void);

static inline Uint16 peek2(const Uint8 *d) { return ((Uint16)d[0] << 8) | d[1]; }
static inline void poke2(Uint8 *d, Uint16 v) { d[0] = v >> 8; d[1] = v; }

static SharpDisplay display;

/*
@|System ------------------------------------------------------------ */

static unsigned int
system_load() {
    memcpy(ram + 0x100, prog_rom, prog_rom_len);
    return 1;
}

static unsigned int
system_boot() {
    return system_load();
}

static void
system_print(char *name, int r)
{
    Uint8 i;
    printf("%s%c", name, ptr[r] - 8 ? ' ' : '|');
    for(i = ptr[r] - 8; i != ptr[r]; i++)
        printf("%02x%c", stk[r][i], i == 0xff ? '|' : ' ');
    printf("<%02x\n", ptr[r]);
}

static Uint8 system_dei_wst(void) { return ptr[0]; }
static Uint8 system_dei_rst(void) { return ptr[1]; }
static void system_deo_wst(void) { ptr[0] = dev[4]; }
static void system_deo_rst(void) { ptr[1] = dev[5]; }
static void system_deo_print(void) { system_print("WST", 0), system_print("RST", 1); }

/*
@|Console ----------------------------------------------------------- */

static int console_vector;

static void console_deo_vector(void) { console_vector = peek2(&dev[0x10]); }
static void console_deo_stdout(void) { putchar_raw(dev[0x18]); }
static void console_deo_stderr(void) { putchar_raw(dev[0x19]); }

static void
console_input(int c, int type)
{
    dev[0x12] = c;
    dev[0x17] = type;
    if(console_vector) uxn_eval(console_vector);
}

/*
@|Devices ----------------------------------------------------------- */

static const dei_handler dei_handlers[256] = {
    [0x04] = system_dei_wst,
    [0x05] = system_dei_rst,
};

static const deo_handler deo_handlers[256] = {
    [0x04] = system_deo_wst,
    [0x05] = system_deo_rst,
    [0x0e] = system_deo_print,
    [0x11] = console_deo_vector,
    [0x18] = console_deo_stdout,
    [0x19] = console_deo_stderr,
};

Uint8 emu_dei(Uint8 port) {
    dei_handler h = dei_handlers[port];
    return h ? h() : dev[port];
}

void emu_deo(Uint8 port, Uint8 value) {
    dev[port] = value;
    deo_handler h = deo_handlers[port];
    if(h) h();
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
