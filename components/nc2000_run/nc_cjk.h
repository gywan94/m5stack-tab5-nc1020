#pragma once
#include <stdint.h>
#define NC_CJK_W 24
#define NC_CJK_H 24
#define NC_CJK_BYTES 72
#define NC_CJK_COUNT 36
/* 24x24 glyph for codepoint cp, or NULL. */
const uint8_t *nc_cjk_glyph(uint32_t cp);
