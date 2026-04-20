// src/io/io.cpp
#include "io/io.h"
#include "video/vdp.h"
#include "audio/psg.h"

IO::IO(VDP& vdp, PSG& psg)
    : vdp(vdp), psg(psg) {}

uint8_t IO::read(uint8_t port) {
    switch (port & 0xFEu) {
        // ── VDP counters ─────────────────────────────────────────────────────
        case 0x40:
            return (port & 0x01u) ? vdp.getHCounter() : vdp.getVCounter();

        // ── VDP data / control ────────────────────────────────────────────────
        case 0xBE:
            return (port & 0x01u) ? vdp.readStatus() : vdp.readData();

        // ── Joypad 1 + joypad 2 lower bits  (0xC0 / 0xC1 and mirrors) ───────
        // 0xC0: bits 5-0 = P1 Fire2..Up, bits 7-6 = P2 Down, P2 Up
        // 0xC1: bits 3-0 = P2 Fire2..Left, upper nibble open bus (0xF0)
        case 0xC0:
            if (port & 0x01u) {
                // 0xC1 / odd — P2 upper buttons + open-bus upper nibble
                return static_cast<uint8_t>(((joypad2 >> 2) & 0x0Fu) | 0xF0u);
            }
            // 0xC0 / even — P1 buttons (bits 5-0) + P2 Up/Down (bits 7-6)
            return static_cast<uint8_t>((joypad1 & 0x3Fu) | ((joypad2 & 0x03u) << 6));

        // ── Mirrors of 0xC0/0xC1 at 0xDC/0xDD ───────────────────────────────
        case 0xDC:
            if (port & 0x01u) {
                return static_cast<uint8_t>(((joypad2 >> 2) & 0x0Fu) | 0xF0u);
            }
            return static_cast<uint8_t>((joypad1 & 0x3Fu) | ((joypad2 & 0x03u) << 6));

        default:
            return 0xFF;  // open bus
    }
}

void IO::write(uint8_t port, uint8_t val) {
    switch (port) {
        case 0x3F:
            ioControl = val;
            break;
        case 0x7E:
        case 0x7F:
            psg.write(val);
            break;
        case 0xBE:
            vdp.writeData(val);
            break;
        case 0xBF:
        case 0xBD:
            vdp.writeControl(val);
            break;
        default:
            break;  // unmapped writes ignored
    }
}

// state is an active-high bitmask (bit set = button pressed);
// invert to active-low for storage and hardware-accurate port reads.
void IO::setJoypad1(uint8_t state) {
    joypad1 = ~state;
}

void IO::setJoypad2(uint8_t state) {
    joypad2 = ~state;
}
