// src/cpu/z80.cpp
#include "cpu/z80.h"
#include "memory/bus.h"

#include <bit>      // std::popcount (C++20/23)
#include <cstdint>

// ── Constructor ──────────────────────────────────────────────────────────────
Z80::Z80(Bus& bus) : bus(bus) {}

// ── reset() ──────────────────────────────────────────────────────────────────
void Z80::reset() {
    regs.PC     = 0x0000;
    regs.SP     = 0xFFFF;
    regs.AF     = 0xFFFF;
    regs.BC     = 0x0000;
    regs.DE     = 0x0000;
    regs.HL     = 0x0000;
    regs.AF_    = 0x0000;
    regs.BC_    = 0x0000;
    regs.DE_    = 0x0000;
    regs.HL_    = 0x0000;
    regs.IX     = 0xFFFF;
    regs.IY     = 0xFFFF;
    regs.I      = 0x00;
    regs.R      = 0x00;
    regs.IFF1   = false;
    regs.IFF2   = false;
    regs.IM     = 0;
    regs.halted = false;
    regs.eiDelay = false;
    pendingIRQ  = false;
}

// ── Memory helpers ────────────────────────────────────────────────────────────
uint8_t Z80::read8(uint16_t addr) {
    return bus.read(addr);
}

void Z80::write8(uint16_t addr, uint8_t val) {
    bus.write(addr, val);
}

uint16_t Z80::read16(uint16_t addr) {
    return static_cast<uint16_t>(read8(addr)) |
           (static_cast<uint16_t>(read8(static_cast<uint16_t>(addr + 1))) << 8);
}

void Z80::write16(uint16_t addr, uint16_t val) {
    write8(addr,                                  static_cast<uint8_t>(val & 0xFF));
    write8(static_cast<uint16_t>(addr + 1),       static_cast<uint8_t>(val >> 8));
}

uint8_t Z80::fetch8() {
    uint8_t val = read8(regs.PC++);
    regs.R = static_cast<uint8_t>((regs.R & 0x80) | ((regs.R + 1) & 0x7F));
    return val;
}

uint16_t Z80::fetch16() {
    uint16_t val = read16(regs.PC);
    regs.PC += 2;
    return val;
}

// ── I/O helpers ───────────────────────────────────────────────────────────────
uint8_t Z80::ioRead(uint8_t port) {
    return bus.ioRead(port);
}

void Z80::ioWrite(uint8_t port, uint8_t val) {
    bus.ioWrite(port, val);
}

// ── Flag helpers ──────────────────────────────────────────────────────────────
bool Z80::parity(uint8_t val) {
    return (std::popcount(val) % 2) == 0;
}

void Z80::setFlagsINC(uint8_t prev, uint8_t result) {
    regs.F.S  = (result & 0x80) != 0;
    regs.F.Z  = (result == 0);
    regs.F.H  = (prev & 0x0F) == 0x0F;
    regs.F.PV = (prev == 0x7F);
    regs.F.N  = 0;
    // C unchanged
    regs.F.F5 = (result & 0x20) != 0;
    regs.F.F3 = (result & 0x08) != 0;
}

void Z80::setFlagsDEC(uint8_t prev, uint8_t result) {
    regs.F.S  = (result & 0x80) != 0;
    regs.F.Z  = (result == 0);
    regs.F.H  = (prev & 0x0F) == 0x00;
    regs.F.PV = (prev == 0x80);
    regs.F.N  = 1;
    // C unchanged
    regs.F.F5 = (result & 0x20) != 0;
    regs.F.F3 = (result & 0x08) != 0;
}

void Z80::setFlagsAdd8(uint8_t a, uint8_t b, bool carry) {
    uint16_t r = static_cast<uint16_t>(a) + b + static_cast<uint16_t>(carry);
    regs.F.S  = (r & 0x80) != 0;
    regs.F.Z  = (r & 0xFF) == 0;
    regs.F.H  = ((a & 0x0F) + (b & 0x0F) + static_cast<uint8_t>(carry)) > 0x0F;
    regs.F.PV = ((~(a ^ b) & (a ^ static_cast<uint8_t>(r)) & 0x80)) != 0;
    regs.F.N  = 0;
    regs.F.C  = r > 0xFF;
    regs.F.F5 = (r & 0x20) != 0;
    regs.F.F3 = (r & 0x08) != 0;
    regs.A    = static_cast<uint8_t>(r);
}

void Z80::setFlagsSub8(uint8_t a, uint8_t b, bool carry) {
    uint16_t r = static_cast<uint16_t>(a) - b - static_cast<uint16_t>(carry);
    regs.F.S  = (r & 0x80) != 0;
    regs.F.Z  = (r & 0xFF) == 0;
    regs.F.H  = (static_cast<int>(a & 0x0F) - static_cast<int>(b & 0x0F)
                 - static_cast<int>(carry)) < 0;
    regs.F.PV = ((a ^ b) & (a ^ static_cast<uint8_t>(r)) & 0x80) != 0;
    regs.F.N  = 1;
    regs.F.C  = r > 0xFF;
    regs.F.F5 = (r & 0x20) != 0;
    regs.F.F3 = (r & 0x08) != 0;
    regs.A    = static_cast<uint8_t>(r);
}

void Z80::setFlagsAnd(uint8_t result) {
    regs.F.S  = (result & 0x80) != 0;
    regs.F.Z  = (result == 0);
    regs.F.H  = 1;
    regs.F.PV = parity(result);
    regs.F.N  = 0;
    regs.F.C  = 0;
    regs.F.F5 = (result & 0x20) != 0;
    regs.F.F3 = (result & 0x08) != 0;
}

void Z80::setFlagsOr(uint8_t result) {
    regs.F.S  = (result & 0x80) != 0;
    regs.F.Z  = (result == 0);
    regs.F.H  = 0;
    regs.F.PV = parity(result);
    regs.F.N  = 0;
    regs.F.C  = 0;
    regs.F.F5 = (result & 0x20) != 0;
    regs.F.F3 = (result & 0x08) != 0;
}

void Z80::setFlagsXor(uint8_t result) {
    regs.F.S  = (result & 0x80) != 0;
    regs.F.Z  = (result == 0);
    regs.F.H  = 0;
    regs.F.PV = parity(result);
    regs.F.N  = 0;
    regs.F.C  = 0;
    regs.F.F5 = (result & 0x20) != 0;
    regs.F.F3 = (result & 0x08) != 0;
}

void Z80::setFlagsCp(uint8_t a, uint8_t b) {
    // Same arithmetic as SUB but result not written to A; F5/F3 come from b
    uint16_t r = static_cast<uint16_t>(a) - b;
    regs.F.S  = (r & 0x80) != 0;
    regs.F.Z  = (r & 0xFF) == 0;
    regs.F.H  = (static_cast<int>(a & 0x0F) - static_cast<int>(b & 0x0F)) < 0;
    regs.F.PV = ((a ^ b) & (a ^ static_cast<uint8_t>(r)) & 0x80) != 0;
    regs.F.N  = 1;
    regs.F.C  = r > 0xFF;
    regs.F.F5 = (b & 0x20) != 0;   // Z80 quirk: from operand, not result
    regs.F.F3 = (b & 0x08) != 0;
}

// ── Interrupt helpers ─────────────────────────────────────────────────────────
void Z80::pushPC() {
    regs.SP -= 2;
    write16(regs.SP, regs.PC);
}

void Z80::handleNMI() {
    regs.IFF2   = regs.IFF1;
    regs.IFF1   = false;
    regs.halted = false;
    pushPC();
    regs.PC = 0x0066;
    // Caller accounts for 11 T-states
}

void Z80::handleIRQ() {
    if (!regs.IFF1)
        return;
    regs.IFF1   = false;
    regs.IFF2   = false;
    regs.halted = false;
    pushPC();
    switch (regs.IM) {
        case 0:
            regs.PC = 0x0038;  // simplified: assume RST 38h on the bus
            break;
        case 1:
            regs.PC = 0x0038;
            break;
        case 2: {
            uint16_t addr = static_cast<uint16_t>((regs.I << 8) | 0xFF);
            regs.PC = read16(addr);
            break;
        }
        default:
            break;
    }
    // Caller accounts for 13 T-states (IM 1)
}

// ── step() ────────────────────────────────────────────────────────────────────
int Z80::step() {
    if (regs.halted)
        return 4;

    // EI delay: the instruction immediately after EI is non-interruptible.
    const bool allowIRQ = !regs.eiDelay;
    if (regs.eiDelay)
        regs.eiDelay = false;

    uint8_t op = fetch8();
    int cycles;
    switch (op) {
        case 0xCB: cycles = executeCB(fetch8()); break;
        case 0xED: cycles = executeED(fetch8()); break;
        case 0xDD: cycles = executeDD(fetch8()); break;
        case 0xFD: cycles = executeFD(fetch8()); break;
        default:   cycles = executeMain(op);     break;
    }

    // Process deferred IRQ — skipped if this instruction was guarded by EI delay.
    if (allowIRQ && pendingIRQ) {
        pendingIRQ = false;
        handleIRQ();   // handleIRQ() is a no-op if IFF1 is false
    }

    return cycles;
}

// ── nmi() / irq() ─────────────────────────────────────────────────────────────
void Z80::nmi() { handleNMI(); }
void Z80::irq() { pendingIRQ = true; }  // deferred: step() will call handleIRQ()

// ── getRegisters() / loadRegisters() ─────────────────────────────────────────
const Z80Registers& Z80::getRegisters()                  const { return regs; }
void Z80::loadRegisters(const Z80Registers& r) {
    regs       = r;
    pendingIRQ = false;  // clear deferred IRQ to prevent spurious interrupt after load
}

// ── Opcode executors implemented in their respective files ───────────────────
// executeMain  → z80_opcodes.cpp
// executeCB    → z80_cb_opcodes.cpp
// executeED    → z80_ed_opcodes.cpp
// executeDD/FD, executeIndexed, executeIndexedCB → z80_ddfd_opcodes.cpp
