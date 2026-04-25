// src/memory/bus.h
#pragma once

#include <cstdint>
#include <vector>

#include "memory/mapper.h"
#include "io/io.h"

class VDP;
class PSG;

// ── System bus ────────────────────────────────────────────────────────────────
//
// Central hub that owns Mapper and IO and routes all CPU memory / I/O
// accesses to the correct subsystem.  VDP and PSG are held by reference;
// they must outlive this Bus instance.
//
class Bus {
public:
    Bus(VDP& vdp, PSG& psg);

    // ── CPU memory interface ──────────────────────────────────────────────────
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t val);

    // ── CPU I/O interface ─────────────────────────────────────────────────────
    uint8_t ioRead(uint8_t port);
    void    ioWrite(uint8_t port, uint8_t val);

    // ── ROM management ────────────────────────────────────────────────────────
    bool loadROM(const std::vector<uint8_t>& data);
    bool isROMLoaded() const;    Mapper::DetectedRegion detectRegion() const;
    void reset();

    // ── Debugger accessors ────────────────────────────────────────────────────
    const Mapper& getMapper() const;    IO&           getIO();    const IO&     getIO()     const;

    // ── VDP wait-state detection ─────────────────────────────────────────────────
    // Returns true (and clears the flag) if the last I/O access targeted
    // VDP ports 0xBE or 0xBF, indicating a potential wait-state is needed.
    bool wasLastIOVDP();

private:
    Mapper mapper;
    IO     io;
    bool   romLoaded      = false;
    mutable bool lastAccessWasVDP = false;
};
