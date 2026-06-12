#pragma once
#include "nc2000_api.h"

/* Run the NC2000 / NC1020 (wangyu wqx) emulator full-screen on the Tab5.
 * `base` is an on-SD path WITHOUT suffix (e.g. "/sd/nc2000/wqx"); `mode`
 * selects which machine + which files (.rom / .nand / .nor) are used. */
void nc2000_run(const char *base, nc2k_mode_t mode);
