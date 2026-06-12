#pragma once
#include <stdint.h>

/* 16x16 CJK bitmap font (YaHei + MingLiU-B/JhengHei fallback), covering the
 * full CJK Unified Ideographs block — simplified AND traditional. Indexed by
 * Unicode codepoint (cjk_codes sorted ascending). Each glyph is 32 bytes:
 * 16 rows x 2 bytes, MSB of byte0 = leftmost pixel.
 * Regenerate with tools/gen_cjk_font16.py. */
#define CJK_GLYPH_W 16
#define CJK_GLYPH_H 16
#define CJK_GLYPH_BYTES 32
#define CJK_GLYPH_COUNT 23940

extern const uint16_t cjk_codes[CJK_GLYPH_COUNT];
extern const uint8_t cjk_bitmaps[CJK_GLYPH_COUNT * CJK_GLYPH_BYTES];

/* Returns pointer to 32-byte glyph for codepoint cp, or NULL. */
const uint8_t *cjk_glyph(uint32_t cp);
