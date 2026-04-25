// src/memory/bus.cpp
#include "memory/bus.h"

Bus::Bus(VDP& vdp, PSG& psg)
    : io(vdp, psg) {}

uint8_t Bus::read(uint16_t addr) {
    return mapper.read(addr);
}

void Bus::write(uint16_t addr, uint8_t val) {
    mapper.write(addr, val);
}

uint8_t Bus::ioRead(uint8_t port) {
    if ((port & 0xFE) == 0xBE) lastAccessWasVDP = true;
    return io.read(port);
}

void Bus::ioWrite(uint8_t port, uint8_t val) {
    if ((port & 0xFE) == 0xBE) lastAccessWasVDP = true;
    io.write(port, val);
}

bool Bus::wasLastIOVDP() {
    bool v = lastAccessWasVDP;
    lastAccessWasVDP = false;
    return v;
}

bool Bus::loadROM(const std::vector<uint8_t>& data) {
    if (!mapper.loadROM(data)) return false;
    romLoaded = true;
    return true;
}

bool Bus::isROMLoaded() const {
    return romLoaded;
}

Mapper::DetectedRegion Bus::detectRegion() const {
    return mapper.detectRegion();
}

void Bus::reset() {
    // mapper.reset() restores default bank registers but preserves ROM data.
    // romLoaded is intentionally kept — a hardware reset does not eject the cartridge.
    mapper.reset();
}

const Mapper& Bus::getMapper() const {
    return mapper;
}

IO& Bus::getIO() {
    return io;
}

const IO& Bus::getIO() const {
    return io;
}
