#pragma once
#include <stdbool.h>
#include "odroid_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/* On-screen virtual gamepad for the Tab5 touch panel (portrait layout):
 * the game is shown in the top half of the 720x1280 panel and these buttons
 * occupy the bottom half. */

/* Draw the button layout into the bottom half of the panel (call once when a
 * game starts, after the display is in portrait mode). Also marks the vpad
 * active so odroid_input_gamepad_read() starts polling it. */
void odroid_vpad_draw(void);

/* Stop polling the vpad (call on game exit). */
void odroid_vpad_disable(void);

bool odroid_vpad_is_active(void);

/* OR the currently-touched virtual buttons into *state (multi-touch). */
void odroid_vpad_poll(odroid_gamepad_state *state);

#ifdef __cplusplus
}
#endif
