// src/system/sms.h
#pragma once

#include "cpu/z80.h"
#include "memory/bus.h"
#include "psg/psg.h"
#include "vdp/vdp.h"
#include <cstdint>
#include <vector>

class SMS {
public:
    SMS();

    // Lifecycle
    void   setRegion(Region r);
    Region getRegion()    const;
    bool   loadROM(const std::vector<uint8_t>& data);
    bool   isROMLoaded()  const;
    void   reset();

    // Run exactly one full frame (CPU + VDP + PSG in sync)
    void   runFrame();

    // Audio: fill stereo float32 buffer for this frame
    void   fillAudio(float* buffer, int numSamples);

    // Input
    void   setJoypad1(uint8_t state);
    void   setJoypad2(uint8_t state);

    // Component access for UI / debugger
    VDP&       getVDP();
    PSG&       getPSG();
    Bus&       getBus();
    Z80&       getCPU();
    const VDP& getVDP() const;
    const PSG& getPSG() const;
    const Bus& getBus() const;
    const Z80& getCPU() const;

private:
    // Declaration order = construction order (critical — members hold refs to each other)
    VDP  vdp;
    PSG  psg;
    Bus  bus;   // Bus(vdp, psg)
    Z80  cpu;   // Z80(bus)

    Region region         = Region::NTSC;
    bool   romLoaded      = false;
    bool   paused         = false;

    int    cyclesPerFrame = 0;  // recomputed by computeCyclesPerFrame()

    void computeCyclesPerFrame();
    // NTSC: 262 lines * 228 cycles = 59736 cycles/frame
    // PAL:  313 lines * 228 cycles = 71364 cycles/frame
};
