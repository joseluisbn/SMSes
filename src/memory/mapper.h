// src/memory/mapper.h
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class Mapper {
public:
    static constexpr uint16_t PAGE_SIZE     = 0x4000;  // 16 KB per slot
    static constexpr uint16_t RAM_SIZE      = 0x2000;  //  8 KB system RAM
    static constexpr uint8_t  MAX_ROM_PAGES = 64;      // 512 KB max (64 × 16 KB)

    // -------------------------------------------------------------------------
    // Region auto-detection from the SMS ROM header at 0x7FF0–0x7FFF.
    // -------------------------------------------------------------------------
    enum class DetectedRegion { NTSC, PAL, Unknown };
    DetectedRegion detectRegion() const;

    // Reset mapper registers to power-on defaults (slot0=0, slot1=1, slot2=2).
    void reset();

    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t val);

    // Direct pointer to system RAM (used by Bus for the 0xC000–0xFFFF region).
    uint8_t*       getRamPtr()       { return ram.data(); }
    const uint8_t* getRamPtr() const { return ram.data(); }

private:
    std::vector<uint8_t>              rom;              // full ROM image
    std::array<uint8_t, RAM_SIZE>     ram{};            // 8 KB system RAM
    std::array<uint8_t, RAM_SIZE>     cartRam{};        // optional cartridge RAM
    std::array<uint8_t, 4>            mapperRegs{};     // mirrors 0xFFFC–0xFFFF
    std::array<uint8_t*, 3>           pages{};          // pointers into rom[] for slots 0-2
    bool                              cartRamEnabled = false;
    std::size_t                       romPageCount   = 0;

    // Recompute pages[] from mapperRegs[1–3].
    void applyMapping();

    // Handle writes to mapper registers at 0xFFFC–0xFFFF.
    void writeMapperReg(uint16_t addr, uint8_t val);
};
