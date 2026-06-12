#pragma once
#include <stdint.h>

/* Tab5 port stub for the SPDS104A DSP / sound chip. The desktop dsp/ folder
 * (the real SP0256-style audio synth) is not vendored; the core only touches
 * three members of the global `dsp` object (dspMode / reset / write) from
 * io_new.cpp, and pc1000bus.h holds it as a bare pointer. A silent no-op DSP is
 * enough to boot and run; real audio can be wired to the BSP codec later. */
class Dsp {
public:
    int dspMode = 0;
    void reset() { dspMode = 0; }
    int  write(int value = 0, int data_low = 0) { (void)value; (void)data_low; return 0; }
};
