#!/usr/bin/env python3
# Render the current VIRT (virtual-keyboard) layout exactly as nc1020_run.c
# composes it (landscape 1280x720), so we can eyeball the LCD area + gaps.
from PIL import Image, ImageDraw, ImageFont

W, H = 1280, 720
KW, RH = 128, 90
KBD_Y = 4 * RH
PAD = 3
# LCD now fills cols 2..7 edge-to-edge (768x360, stretched 4.8x/4.5x)
LCD_W, LCD_H = 768, 360
LCD_LX = 2 * KW   # 256
LCD_LY = 0

def rgb565_to_rgb(c):
    r = ((c >> 11) & 0x1f) << 3
    g = ((c >> 5) & 0x3f) << 2
    b = (c & 0x1f) << 3
    return (r, g, b)

KC_KEY  = rgb565_to_rgb(0x4a49)
KC_APP  = rgb565_to_rgb(0x82A6)
KC_FN   = rgb565_to_rgb(0x6B4D)
KC_SAVE = rgb565_to_rgb(0x05E0)
KC_FF   = rgb565_to_rgb(0xC340)
KC_TGL  = rgb565_to_rgb(0x051F)

def kcolor(code):
    if code == 0xFD: return KC_SAVE
    if code == 0xFA: return KC_TGL
    if code in (0xFC, 0xFB): return KC_FF
    if 0x08 <= code <= 0x0f: return KC_APP
    if 0x10 <= code <= 0x13: return KC_FN
    return KC_KEY

NONE = (0xFF, "")
KB = [
  [(0x0f,"电源"),(0x0b,"英汉"),NONE,NONE,NONE,NONE,NONE,NONE,(0xFD,"保存"),(0xFA,"全屏")],
  [(0x0c,"名片"),(0x0d,"计算"),NONE,NONE,NONE,NONE,NONE,NONE,(0xFC,"加速"),(0xFB,"还原")],
  [(0x0a,"行程"),(0x09,"测验"),NONE,NONE,NONE,NONE,NONE,NONE,(0x10,"F1"),(0x11,"F2")],
  [(0x08,"其他"),(0x0e,"网络"),NONE,NONE,NONE,NONE,NONE,NONE,(0x12,"F3"),(0x13,"F4")],
  [(0x20,"Q"),(0x21,"W"),(0x22,"E"),(0x23,"R"),(0x24,"T 7"),(0x25,"Y 8"),(0x26,"U 9"),(0x27,"I"),(0x18,"O"),(0x1c,"P")],
  [(0x28,"A"),(0x29,"S"),(0x2a,"D"),(0x2b,"F"),(0x2c,"G 4"),(0x2d,"H 5"),(0x2e,"J 6"),(0x2f,"K"),(0x19,"L"),(0x1d,"Enter")],
  [(0x30,"Z"),(0x31,"X"),(0x32,"C"),(0x33,"V"),(0x34,"B 1"),(0x35,"N 2"),(0x36,"M 3"),(0x37,"PgUp"),(0x1a,"↑"),(0x1e,"PgDn")],
  [(0x38,"Help"),(0x39,"Shift"),(0x3a,"Caps"),(0x3b,"Esc"),(0x3c,"0"),(0x3d,"."),(0x3e,"="),(0x3f,"←"),(0x1b,"↓"),(0x1f,"→")],
]

img = Image.new("RGB", (W, H), (0, 0, 0))
d = ImageDraw.Draw(img)
cjk = ImageFont.truetype(r"C:\Windows\Fonts\msyh.ttc", 26, index=0)
asc = ImageFont.truetype(r"C:\Windows\Fonts\consola.ttf", 26)

for r in range(8):
    y = r * RH if r < 4 else KBD_Y + (r - 4) * RH
    for c in range(10):
        code, lab = KB[r][c]
        if code == 0xFF:
            continue
        x = c * KW
        col = kcolor(code)
        d.rectangle([x+PAD, y+PAD, x+KW-PAD, y+RH-PAD], fill=col)
        f = cjk if (lab and ord(lab[0]) >= 0x80) else asc
        bb = d.textbbox((0,0), lab, font=f)
        tw, th = bb[2]-bb[0], bb[3]-bb[1]
        d.text((x+KW/2-tw/2-bb[0], y+RH/2-th/2-bb[1]), lab, fill=(255,255,255), font=f)

# white LCD backdrop (cols 2..7, rows 0..3) and the actual LCD rect
d.rectangle([2*KW, 0, 8*KW, 4*RH], fill=(255,255,255))
d.rectangle([LCD_LX, LCD_LY, LCD_LX+LCD_W, LCD_LY+LCD_H], outline=(0,0,0), width=3)
d.text((LCD_LX+20, 20), "NC1020 LCD 160x80 -> 720x360 (4.5x)", fill=(0,0,0), font=asc)
# mark the empty white margins
d.rectangle([2*KW, 0, LCD_LX, 4*RH], outline=(255,0,0), width=2)
d.rectangle([LCD_LX+LCD_W, 0, 8*KW, 4*RH], outline=(255,0,0), width=2)

img.save(r"C:\Users\gywan\Documents\trae-project\tab5-retro\tab-nc1020\assets\layout_virt.png")
print("saved assets/layout_virt.png", img.size)
print(f"LCD cell cols2-7 = {6*KW}px wide, LCD = {LCD_W}px -> side margin {(6*KW-LCD_W)//2}px each (red boxes)")
