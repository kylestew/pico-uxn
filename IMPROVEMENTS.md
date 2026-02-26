# Improvements

## Dirty-Region Partial Refresh

The current `sharp_display_refresh` sends every line of the framebuffer over SPI on every call. For a 400x240 display this is 12,000 bytes plus line-address overhead per refresh — even if only a handful of pixels changed.

### Approach

Track which lines have been modified since the last refresh using a per-line dirty bitmap:

```c
uint8_t dirty[(SHARP_HEIGHT + 7) / 8]; // 30 bytes for 240 lines
```

When framebuffer pixels are written, mark the corresponding line as dirty. On refresh, only transmit dirty lines:

```c
void sharp_display_refresh_dirty(SharpDisplay *d) {
    gpio_put(d->cs_pin, 1);
    busy_wait_us_32(6);

    uint8_t byte = SHARP_BIT_WRITECMD | d->vcom;
    spi_write_blocking(d->spi, &byte, 1);

    for (uint16_t y = 0; y < SHARP_HEIGHT; y++) {
        if (!(d->dirty[y / 8] & (1 << (y % 8)))) continue;

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
    memset(d->dirty, 0, sizeof(d->dirty));
}
```

The Sharp Memory LCD protocol allows sending an arbitrary subset of lines in any single write command, so skipping clean lines is fully supported. This turns a static display into a near-zero-cost operation and keeps partial UI updates proportional to the area that actually changed.

### Pixel Write Helper

A `sharp_display_set_pixel` (or byte-level equivalent) should set the dirty bit automatically so callers never need to think about it:

```c
static inline void sharp_display_set_pixel(SharpDisplay *d, uint16_t x, uint16_t y, bool on) {
    uint16_t idx = y * SHARP_WIDTH_BYTES + (x / 8);
    uint8_t mask = 1 << (x % 8);
    uint8_t old  = d->framebuffer[idx];
    d->framebuffer[idx] = on ? (old | mask) : (old & ~mask);
    if (d->framebuffer[idx] != old)
        d->dirty[y / 8] |= (1 << (y % 8));
}
```

---

## Timer-Based VCOM Toggle

The Sharp Memory LCD requires VCOM to be toggled at least once per second to prevent DC bias damage. Right now VCOM is toggled as a side effect of `sharp_display_refresh` and `sharp_display_clear`, which works only as long as one of those is called frequently enough. If the display is idle (nothing dirty, no refresh calls), VCOM stops toggling and the panel may degrade.

### Approach

Use a Pico SDK repeating timer to toggle VCOM independently of refresh calls:

```c
static bool vcom_timer_cb(repeating_timer_t *rt) {
    SharpDisplay *d = (SharpDisplay *)rt->user_data;
    uint8_t cmd[2] = {d->vcom, 0x00};

    gpio_put(d->cs_pin, 1);
    busy_wait_us_32(6);
    spi_write_blocking(d->spi, cmd, 2);
    gpio_put(d->cs_pin, 0);
    busy_wait_us_32(2);

    d->vcom ^= SHARP_BIT_VCOM;
    return true; // keep repeating
}
```

Start the timer after init:

```c
repeating_timer_t vcom_timer;
add_repeating_timer_ms(-1000, vcom_timer_cb, &display, &vcom_timer);
```

Using a negative interval guarantees the period is measured from the _start_ of each callback, keeping the toggle rate stable regardless of callback duration.

### Considerations

- The timer callback fires from the timer interrupt context. If `sharp_display_refresh_dirty` could be running concurrently on the same SPI bus, a lightweight guard (a `volatile bool spi_busy` flag or a critical section) is needed to prevent bus contention.
- Alternatively, the timer can simply set a flag that the main loop checks, keeping all SPI access single-threaded:

```c
static volatile bool vcom_due;

static bool vcom_timer_cb(repeating_timer_t *rt) {
    vcom_due = true;
    return true;
}

// In the main loop
if (vcom_due) {
    vcom_due = false;
    sharp_display_toggle_vcom(&display);
}
```

This flag-based approach avoids any concurrency issues and is the safer default for a single-core main-loop architecture.
