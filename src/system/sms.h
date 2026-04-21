// src/system/sms.h
#pragma once

#include "vdp/vdp.h"

#include <cstdint>
#include <vector>

class SMS {
public:
    SMS();

    void   setRegion(Region r);
    Region getRegion() const;

    bool loadROM(const std::vector<uint8_t>& data);
    void reset();
    void runFrame();    // execute one full frame worth of CPU + VDP cycles

private:
    Region region = Region::NTSC;

    // Components owned by SMS — instantiated in Phase 6:
    // VDP vdp;
    // PSG psg;
    // Bus bus;
    // Z80 cpu;
};
