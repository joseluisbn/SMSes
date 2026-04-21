// tests/mock_bus.h
#pragma once

#include "memory/bus.h"

#include <array>
#include <cstdint>
#include <vector>

// ── MockBus ───────────────────────────────────────────────────────────────────
// Flat 64 KB RAM — load ROM bytes at any address, inspect RAM after execution.
class MockBus : public Bus {
public:
    std::array<uint8_t, 65536> memory{};

    // Load bytes at base address (default 0x0000)
    void load(const std::vector<uint8_t>& bytes, uint16_t base = 0) {
        for (std::size_t i = 0; i < bytes.size() && (base + i) <= 0xFFFFu; ++i)
            memory[static_cast<uint16_t>(base + i)] = bytes[i];
    }

    void loadAt(uint16_t addr, const std::vector<uint8_t>& bytes) {
        load(bytes, addr);
    }

    uint8_t read(uint16_t addr)               override { return memory[addr]; }
    void    write(uint16_t addr, uint8_t val) override { memory[addr] = val; }
    uint8_t ioRead(uint8_t)                   override { return 0xFF; }
    void    ioWrite(uint8_t, uint8_t)         override {}
};
