/*
 * Minimal C reader for the M5Stack Tab5 hardware keyboard (detachable case).
 *
 * The keyboard is an I2C peripheral (addr 0x6D) on its own bus: I2C_NUM_1,
 * SDA=GPIO0, SCL=GPIO1. In NORMAL mode it exposes a 1-byte key event at
 * register 0x20 encoded as: bit7 = pressed, bits6..4 = row, bits3..0 = col.
 * This avoids pulling in the full C++ driver + i2c_bus dependency.
 */
#ifndef KBD_HW_H
#define KBD_HW_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t row;
    uint8_t col;
    bool    pressed;
} kbd_hw_event_t;

/* Bring up the keyboard in NORMAL (row/col) mode. Returns true if the device
 * acknowledged on I2C (i.e. the keyboard case is attached). Safe to ignore the
 * result — poll just returns 0 when no keyboard is present. */
bool kbd_hw_init(void);

/* Drain pending key events into `evs` (up to `max`); returns the count. */
int kbd_hw_poll(kbd_hw_event_t *evs, int max);

void kbd_hw_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* KBD_HW_H */
