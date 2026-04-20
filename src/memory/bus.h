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
    bool isROMLoaded() const;

    void reset();

    // ── Debugger accessors ────────────────────────────────────────────────────
    const Mapper& getMapper() const;
    const IO&     getIO()     const;

private:
    Mapper mapper;
    IO     io;
    bool   romLoaded = false;
};
