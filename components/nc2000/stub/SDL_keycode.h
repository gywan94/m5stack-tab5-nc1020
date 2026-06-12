#pragma once
#include <stdint.h>
typedef int32_t SDL_Keycode;

/* Minimal SDL keycode constants for the ESP32 port. The desktop key-mapping
 * functions (map_key/handle_key in key.cpp) are compiled but unused — the Tab5
 * glue calls SetKey() directly — so these values only need to be distinct.
 * ASCII-printable keys use their character code; the rest get codes >= 0x100. */
enum {
    SDLK_BACKSPACE = 8,
    SDLK_TAB       = 9,
    SDLK_RETURN    = 13,
    SDLK_ESCAPE    = 27,
    SDLK_SPACE     = ' ',
    SDLK_QUOTE     = '\'',
    SDLK_COMMA     = ',',
    SDLK_MINUS     = '-',
    SDLK_PERIOD    = '.',
    SDLK_SLASH     = '/',
    SDLK_0 = '0', SDLK_1, SDLK_2, SDLK_3, SDLK_4,
    SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9,
    SDLK_SEMICOLON = ';',
    SDLK_EQUALS    = '=',
    SDLK_LEFTBRACKET  = '[',
    SDLK_BACKSLASH    = '\\',
    SDLK_RIGHTBRACKET = ']',
    SDLK_BACKQUOTE    = '`',
    SDLK_a = 'a', SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f, SDLK_g,
    SDLK_h, SDLK_i, SDLK_j, SDLK_k, SDLK_l, SDLK_m, SDLK_n,
    SDLK_o, SDLK_p, SDLK_q, SDLK_r, SDLK_s, SDLK_t, SDLK_u,
    SDLK_v, SDLK_w, SDLK_x, SDLK_y, SDLK_z,

    SDLK_RIGHT = 0x100, SDLK_LEFT, SDLK_DOWN, SDLK_UP,
    SDLK_LSHIFT, SDLK_RSHIFT, SDLK_LCTRL, SDLK_RCTRL,
    SDLK_LALT, SDLK_RALT,
    SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6,
    SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12,
};
