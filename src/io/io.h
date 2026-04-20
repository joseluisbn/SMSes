// src/io/io.h
#pragma once

#include <cstdint>

class VDP;
class PSG;

// ── Joypad button bitmask constants (active-low) ─────────────────────────────
inline constexpr uint8_t JOY_UP    = 0x01;
inline constexpr uint8_t JOY_DOWN  = 0x02;
inline constexpr uint8_t JOY_LEFT  = 0x04;
inline constexpr uint8_t JOY_RIGHT = 0x08;
inline constexpr uint8_t JOY_FIRE1 = 0x10;
inline constexpr uint8_t JOY_FIRE2 = 0x20;

// ── I/O port handler ─────────────────────────────────────────────────────────
//
// Port map (SMS):
//   0x3F  (write) — I/O control register
//   0x40  (read)  — VDP V-counter
//   0x41  (read)  — VDP H-counter
//   0x7E  (write) — PSG data
//   0x7F  (write) — PSG data (mirror)
//   0xBE  (read/write) — VDP data port
//   0xBF  (read/write) — VDP control port
//   0xC0 / 0xDC  (read) — Joypad 1 + joypad 2 lower bits
//   0xC1 / 0xDD  (read) — Joypad 2 + misc buttons
//
class IO {
public:
    IO(VDP& vdp, PSG& psg);

    uint8_t read(uint8_t port);
    void    write(uint8_t port, uint8_t val);

    // Called by App input handling each frame.
    // state is an active-low bitmask using JOY_* constants.
    void setJoypad1(uint8_t state);
    void setJoypad2(uint8_t state);

private:
    VDP& vdp;
    PSG& psg;

    uint8_t joypad1   = 0xFF;  // all buttons released (active-low)
    uint8_t joypad2   = 0xFF;
    uint8_t ioControl = 0xFF;  // port 0x3F — I/O port control register

    // Port 0xC0 bit layout:
    //   bit 0: P1 Up    bit 1: P1 Down  bit 2: P1 Left  bit 3: P1 Right
    //   bit 4: P1 Fire1 bit 5: P1 Fire2 bit 6: P2 Up    bit 7: P2 Down
};
