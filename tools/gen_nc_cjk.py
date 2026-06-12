#!/usr/bin/env python3
# Generate components/nc1020_run/nc_cjk.{c,h} — a tiny 24x24 CJK bitmap font
# with only the Chinese characters used on the NC1020 on-screen UI (both the
# hardware-mode function strip and the virtual keyboard, plus the loading text).
# Rendered from Microsoft YaHei (msyh.ttc) with Pillow. Run from tab-nc1020/.
#
# Format: each glyph = 24 rows x 3 bytes (MSB of byte0 = leftmost pixel).
# Lookup by Unicode codepoint via nc_cjk_glyph() (binary search).

import os
from PIL import Image, ImageFont, ImageDraw

GLYPH = 24
BYTES_PER_ROW = (GLYPH + 7) // 8      # 3
GLYPH_BYTES = GLYPH * BYTES_PER_ROW   # 72
THRESHOLD = 110
FONT = (r"C:\Windows\Fonts\msyh.ttc", 0)

# app/util keys + QWERTY function labels + toggle keys + loading text
CHARS = "电源英汉名片计算行程测验其他网络保存切换加速还原帮助上页下页全屏软键载入中"

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_C = os.path.normpath(os.path.join(HERE, "..", "components", "nc1020_run", "nc_cjk.c"))
OUT_H = os.path.normpath(os.path.join(HERE, "..", "components", "nc1020_run", "nc_cjk.h"))


def render(face, ch):
    img = Image.new("L", (GLYPH, GLYPH), 0)
    d = ImageDraw.Draw(img)
    bb = d.textbbox((0, 0), ch, font=face)
    w, h = bb[2] - bb[0], bb[3] - bb[1]
    ox = (GLYPH - w) // 2 - bb[0]
    oy = (GLYPH - h) // 2 - bb[1]
    d.text((ox, oy), ch, fill=255, font=face)
    px = img.load()
    out = bytearray()
    for y in range(GLYPH):
        for b in range(BYTES_PER_ROW):
            v = 0
            for bit in range(8):
                x = b * 8 + bit
                if x < GLYPH and px[x, y] >= THRESHOLD:
                    v |= (0x80 >> bit)
            out.append(v)
    return bytes(out)


def main():
    face = ImageFont.truetype(FONT[0], GLYPH, index=FONT[1])
    uniq = []
    for c in CHARS:
        if c not in uniq:
            uniq.append(c)
    uniq.sort(key=ord)

    codes = [ord(c) for c in uniq]
    glyphs = [render(face, c) for c in uniq]

    h = []
    h.append("#pragma once\n#include <stdint.h>\n")
    h.append(f"#define NC_CJK_W {GLYPH}\n#define NC_CJK_H {GLYPH}\n")
    h.append(f"#define NC_CJK_BYTES {GLYPH_BYTES}\n")
    h.append(f"#define NC_CJK_COUNT {len(codes)}\n")
    h.append("/* 24x24 glyph for codepoint cp, or NULL. */\n")
    h.append("const uint8_t *nc_cjk_glyph(uint32_t cp);\n")
    open(OUT_H, "w", encoding="utf-8", newline="\n").write("".join(h))

    c = []
    c.append('#include "nc_cjk.h"\n\n')
    c.append(f"static const uint16_t s_codes[{len(codes)}] = {{\n    ")
    c.append(",".join(f"0x{x:04x}" for x in codes))
    c.append("\n};\n\n")
    c.append(f"static const uint8_t s_bitmaps[{len(codes)} * NC_CJK_BYTES] = {{\n")
    for g in glyphs:
        c.append("    " + ",".join(f"0x{b:02x}" for b in g) + ",\n")
    c.append("};\n\n")
    c.append("""const uint8_t *nc_cjk_glyph(uint32_t cp)
{
    int lo = 0, hi = NC_CJK_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        uint16_t v = s_codes[mid];
        if (v == cp) return &s_bitmaps[mid * NC_CJK_BYTES];
        if (v < cp) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}
""")
    open(OUT_C, "w", encoding="utf-8", newline="\n").write("".join(c))
    print(f"generated nc_cjk: {len(codes)} glyphs, {len(codes)*GLYPH_BYTES} bytes")


if __name__ == "__main__":
    main()
