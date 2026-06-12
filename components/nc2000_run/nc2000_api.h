/*
 * nc2000_api.h — thin C-callable shim over the wangyu NC2000 C++ core.
 *
 * The display / keyboard / PPA glue (nc2000_run.c) is plain C and the emulator
 * core is C++; this header is the boundary. nc2000_api.cpp sets the core's
 * global mode switches + file paths and forwards the emulator API.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NC2K_MODE_NC2000 = 0,   /* NAND-based: <base>.nand + .nand0 + .nor   */
    NC2K_MODE_NC1020 = 1,   /* ROM-based:  <base>.rom + .nor             */
    NC2K_MODE_NC3000 = 2,   /* double-NAND                               */
    NC2K_MODE_PC1000 = 3,   /* ROM-based                                 */
} nc2k_mode_t;

/* Configure the core for `mode`, deriving every file path from `base` (a path
 * WITHOUT suffix, e.g. "/sd/nc2000/wqx"). Sets cpu_loop_version/io_version and
 * calls init_parameters(). Must be called before nc2k_load(). */
void nc2k_configure(nc2k_mode_t mode, const char *base);

/* Load ROM/NOR/NAND from the configured paths and start the machine.
 * Returns false (without loading) if a file the mode requires is missing. */
bool nc2k_load(void);

/* Emulate one time-slice of `ms` milliseconds (fast = uncapped speed). */
void nc2k_run_slice(uint32_t ms, bool fast);

/* Copy the 160x80 LCD into `buf`. Returns:
 *   0 = no buffer this frame, 1 = 1bpp (1600 B), 2 = 2bpp grey (3200 B). */
int nc2k_copy_lcd(uint8_t *buf);

/* Press/release a key (codes match key.cpp SetKey, same scheme as NC1020). */
void nc2k_set_key(uint8_t code, bool down);

/* Persist NOR + NAND + saved state to their files. */
void nc2k_save(void);

#ifdef __cplusplus
}
#endif
