// src/memory/mapper.cpp
#include "memory/mapper.h"

#include <cstddef>
#include <cstdint>

// ── loadROM() ────────────────────────────────────────────────────────────────
bool Mapper::loadROM(const std::vector<uint8_t>& data) {
    if (data.empty())
        return false;
    if (data.size() % PAGE_SIZE != 0)
        return false;
    if (data.size() > static_cast<std::size_t>(MAX_ROM_PAGES) * PAGE_SIZE)
        return false;

    rom          = data;
    romPageCount = rom.size() / PAGE_SIZE;
    reset();
    return true;
}

// ── reset() ──────────────────────────────────────────────────────────────────
void Mapper::reset() {
    mapperRegs      = {0, 0, 1, 2};
    cartRamEnabled  = false;
    applyMapping();
}

// ── applyMapping() ────────────────────────────────────────────────────────────
void Mapper::applyMapping() {
    if (romPageCount == 0)
        return;  // no ROM loaded yet — leave pages[] uninitialised
    for (int i = 0; i < 3; ++i) {
        std::size_t page = static_cast<std::size_t>(mapperRegs[i + 1]) % romPageCount;
        pages[static_cast<std::size_t>(i)] = &rom[page * PAGE_SIZE];
    }
}

// ── read() ────────────────────────────────────────────────────────────────────
uint8_t Mapper::read(uint16_t addr) {
    if (addr <= 0x03FFu) {
        // First 1 KB is always the first page of ROM, bypasses mapper
        return rom[addr];
    }
    if (addr <= 0x3FFFu) {
        return pages[0][addr];                        // slot 0: 0x0000–0x3FFF
    }
    if (addr <= 0x7FFFu) {
        return pages[1][addr - 0x4000u];              // slot 1: 0x4000–0x7FFF
    }
    if (addr <= 0xBFFFu) {
        if (cartRamEnabled)
            return cartRam[addr & 0x1FFFu];           // cart RAM
        return pages[2][addr - 0x8000u];              // slot 2: 0x8000–0xBFFF
    }
    // 0xC000–0xFFFF — system RAM (0xE000–0xFFFF mirrors 0xC000–0xDFFF)
    return ram[addr & 0x1FFFu];
}

// ── write() ───────────────────────────────────────────────────────────────────
void Mapper::write(uint16_t addr, uint8_t val) {
    if (addr <= 0x7FFFu) {
        return;                                        // ROM — ignore
    }
    if (addr <= 0xBFFFu) {
        if (cartRamEnabled)
            cartRam[addr & 0x1FFFu] = val;
        return;
    }
    // 0xC000–0xFFFF — system RAM (and mapper registers at 0xFFFC–0xFFFF)
    ram[addr & 0x1FFFu] = val;
    if (addr >= 0xFFFCu)
        writeMapperReg(addr, val);
}

// ── writeMapperReg() ──────────────────────────────────────────────────────────
void Mapper::writeMapperReg(uint16_t addr, uint8_t val) {
    mapperRegs[addr & 3u] = val;
    cartRamEnabled = (mapperRegs[0] & 0x08u) != 0;
    applyMapping();
}
