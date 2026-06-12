#pragma once
#include <stdint.h>
/* ASCII 0x20..0x7E, 8x16, MSB=leftmost. font8[c-0x20][row]. */
extern const uint8_t vpad_font8[95][16];
