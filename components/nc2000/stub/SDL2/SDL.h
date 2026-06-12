#pragma once
/* Minimal SDL2 shim for the ESP32 port — the desktop GUI/audio is replaced by
 * the Tab5 glue, so only the few symbols the core references are provided. */
#include <stdint.h>
#include <SDL_keycode.h>
#include <SDL_keyboard.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SDL_Window SDL_Window;
uint32_t SDL_GetTicks(void);
uint64_t SDL_GetTicks64(void);
void SDL_SetWindowTitle(SDL_Window*, const char*);
#ifdef __cplusplus
}
#endif
