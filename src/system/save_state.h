// src/system/save_state.h
#pragma once

// Save-state POD structs for all SMS components.
// Only included by system/sms.h, sms.cpp, and component .cpp files.
// Component .h files forward-declare these structs to avoid circular includes.

#include "cpu/z80.h"
#include "psg/psg.h"
#include "vdp/vdp.h"
#include <array>
#include <cstdint>

// ── VDP ───────────────────────────────────────────────────────────────────────

struct VDPSaveState {
    std::array<uint8_t, 16384> vram;
    std::array<uint8_t,    32> cram;
    std::array<uint8_t,    16> regs;
    uint16_t addrReg;
    uint8_t  codeReg;
    bool     firstByte;
    uint8_t  readBuffer;
    uint8_t  statusReg;
    bool     irqPending;
    int      currentLine;
    int      cycleCounter;
    int      lineCounter;
    uint8_t  vScroll;
    int      hCycleCounter;
    uint8_t  region;          // stored as uint8_t to avoid int-alignment padding
    int      linesPerFrame;
};

// ── PSG ───────────────────────────────────────────────────────────────────────

struct PSGSaveState {
    ToneChannel  tone[3];
    NoiseChannel noise;
    uint8_t      latchedChannel;
    bool         latchedIsVolume;
    double       clockHz;
};

// ── Mapper ────────────────────────────────────────────────────────────────────

struct MapperSaveState {
    std::array<uint8_t, 8192> ram;
    std::array<uint8_t, 8192> cartRam;
    std::array<uint8_t,    4> mapperRegs;
    bool                      cartRamEnabled;
};

// ── Top-level save state ──────────────────────────────────────────────────────

struct SaveState {
    static constexpr uint32_t MAGIC   = 0x534D5353u;  // "SMSS"
    static constexpr uint32_t VERSION = 1u;

    uint32_t        magic   = MAGIC;
    uint32_t        version = VERSION;
    Z80Registers    cpu;
    VDPSaveState    vdp;
    PSGSaveState    psg;
    MapperSaveState mapper;
    uint8_t         region;   // stored as uint8_t to avoid int-alignment padding
};
