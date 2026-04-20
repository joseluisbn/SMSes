// src/memory/bus.h
#pragma once

#include <cstdint>

// Abstract memory and I/O bus interface.
// The concrete implementation lives in bus.cpp (Phase 3 — Memory).
// CPU, VDP, and PSG receive a Bus& injected by the system.
class Bus {
public:
    virtual ~Bus() = default;

    virtual uint8_t read(uint16_t addr)               = 0;
    virtual void    write(uint16_t addr, uint8_t val) = 0;
    virtual uint8_t ioRead(uint8_t port)              = 0;
    virtual void    ioWrite(uint8_t port, uint8_t val) = 0;
};
