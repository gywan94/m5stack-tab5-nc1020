/*
 * nc2000_api.cpp — C-callable shim implementation. Mirrors the relevant parts
 * of the desktop settings.cpp (mode globals + WqxRom path derivation) and
 * main.cpp (init order) without the SDL/argv layer.
 */
#include <string>
#include <unistd.h>
#include <cstdio>
#include <sys/stat.h>

#include "comm.h"
#include "nc2000.h"
#include "io.h"
#include "key_new.h"
#include "nor.h"
#include "state.h"

#include "nc2000_api.h"

extern WqxRom nc2k_rom;
extern nc2k_states_t nc2k_states;
void init_keyitems();                                   /* key_new.cpp      */
void SetKeyWayback(int code_y, int code_x, bool down);  /* key_new.cpp      */
void warm_reset_if_clkoff();                            /* cpu_loop_new.cpp */

extern "C" void nc2k_configure(nc2k_mode_t mode, const char *base)
{
    std::string b = base ? base : "";

    nc1020mode = nc2000mode = nc3000mode = pc1000mode = nc1020tw_mode = false;
    cpu_loop_version = CPU_RUN3;
    io_version       = IO_V2;
    nc2k_rom = WqxRom();

    switch (mode) {
        case NC2K_MODE_NC2000:
            nc2000mode = true;
            nc2k_rom.nandFlashPath = b + ".nand";
            nc2k_rom.nand0Path     = b + ".nand0";
            nc2k_rom.norFlashPath  = b + ".nor";
            break;
        case NC2K_MODE_NC3000:
            nc3000mode = true;
            nc2k_rom.nand0Path     = b + ".nand0";
            nc2k_rom.nandFlashPath = b + ".nand";
            nc2k_rom.norFlashPath  = b + ".nor";
            break;
        case NC2K_MODE_NC1020:
            nc1020mode = true;
            nc2k_rom.romPath      = b + ".rom";
            nc2k_rom.norFlashPath = b + ".nor";
            /* desktop settings.cpp seeds these two NOR info bytes for nc1020 */
            nor_info_block[8] = 0xfc;
            nor_info_block[9] = 0x03;
            break;
        case NC2K_MODE_PC1000:
            pc1000mode = true;
            nc2k_rom.romPath      = b + ".rom";
            nc2k_rom.norFlashPath = b + ".nor";
            break;
    }
    nc2k_rom.statesPath = b + ".state";

    /* Persist NOR/NAND back to their files when we exit. */
    save_flash_on_exit = true;

    init_parameters();
    /* Larger CPU batch than the desktop default (64) — the per-batch overhead of
     * cpu_run3() dominates on the P4 (ROM/NOR live in slower PSRAM). Timers use
     * count-based trigger_x_times_per_s(), so coarser batching stays correct. */
    extern uint32_t cpu_batch;
    cpu_batch = 4096;
    init_keyitems();
}

extern "C" bool nc2k_load(void)
{
    /* Pre-check the files the chosen mode needs, so a missing data file fails
     * cleanly here instead of exit()-ing deep in the core (which reboot-loops).
     * The writable .nor is optional (a blank one is created on first save). */
    if (nc1020mode || pc1000mode) {
        if (access(nc2k_rom.romPath.c_str(), F_OK) != 0) {
            printf("[wqx] required ROM missing: %s\n", nc2k_rom.romPath.c_str());
            return false;
        }
    }
    if (nc2000mode || nc3000mode) {
        if (access(nc2k_rom.nandFlashPath.c_str(), F_OK) != 0 ||
            access(nc2k_rom.nand0Path.c_str(),     F_OK) != 0) {
            printf("[wqx] required NAND files missing: %s / %s\n",
                   nc2k_rom.nandFlashPath.c_str(), nc2k_rom.nand0Path.c_str());
            return false;
        }
    }
    /* Diagnostic: report the actual sizes of the data files we're loading. */
    {
        struct stat st;
        if (nc1020mode || pc1000mode) {
            if (stat(nc2k_rom.romPath.c_str(), &st) == 0)
                printf("[wqx] ROM %s = %ld bytes (expect 12582912)\n",
                       nc2k_rom.romPath.c_str(), (long)st.st_size);
        }
        if (stat(nc2k_rom.norFlashPath.c_str(), &st) == 0)
            printf("[wqx] NOR %s = %ld bytes (expect 524288)\n",
                   nc2k_rom.norFlashPath.c_str(), (long)st.st_size);
        else
            printf("[wqx] NOR %s MISSING -> blank (may not boot!)\n",
                   nc2k_rom.norFlashPath.c_str());
    }
    LoadNC2k();
    return true;
}

extern "C" void nc2k_run_slice(uint32_t ms, bool fast)
{
    fast_forward = fast;
    RunTimeSlice(ms);
}

extern "C" int nc2k_copy_lcd(uint8_t *buf)
{
    int grey = is_grey_mode() ? 1 : 0;
    if (!CopyLcdBuffer(buf)) return 0;
    return grey ? 2 : 1;
}

extern "C" void nc2k_set_key(uint8_t code, bool down)
{
    /* With IO_V2 (the io_version we run), the keyboard is scanned through
     * NekoDriverIO's keypadmatrix[8][8] — written ONLY by SetKeyWayback().
     * key.cpp's SetKey() writes the legacy keypad_matrix nothing reads here,
     * which is why keys silently did nothing. Mirror handle_key_wayback():   */
    if (nc1020mode && code == 0x0f) {
        /* nc1020 power key is an independent pin, not on the scan matrix:
         * ram_io[0x0b] bit0, active low (desktop maps it to F12). */
        uint8_t *ram_io = nc2k_states.ram_io;
        if (down) {
            ram_io[0x0b] &= (uint8_t)~1;
            warm_reset_if_clkoff();
        } else {
            ram_io[0x0b] |= 1;
        }
        return;
    }
    if (nc2000mode && code == 0x0f) {
        SetKeyWayback(0, 0, down);   /* nc2000 ON/OFF sits at matrix (0,0) */
        return;
    }
    /* Standard wqx code -> matrix position (same encoding SetKey used). */
    SetKeyWayback(code % 8, code / 8, down);
}

extern "C" void nc2k_save(void)
{
    save_flash("");
    save_state("");
}
