// src/cpu/z80.h
#pragma once

#include <cstdint>

class Bus;

// ── Flags register (F) ───────────────────────────────────────────────────────
// Bitfield order matches Z80 specification on little-endian targets (GCC/Clang/MSVC x86-64).
union Flags {
    uint8_t raw = 0;
    struct {
        uint8_t C  : 1;  // bit 0 — Carry
        uint8_t N  : 1;  // bit 1 — Add/Subtract
        uint8_t PV : 1;  // bit 2 — Parity/Overflow
        uint8_t F3 : 1;  // bit 3 — undocumented (copy of result bit 3)
        uint8_t H  : 1;  // bit 4 — Half Carry
        uint8_t F5 : 1;  // bit 5 — undocumented (copy of result bit 5)
        uint8_t Z  : 1;  // bit 6 — Zero
        uint8_t S  : 1;  // bit 7 — Sign
    };
};

// ── Z80 register file ────────────────────────────────────────────────────────
struct Z80Registers {
    // Main register pairs — accessible as 16-bit or individual hi/lo bytes.
    // On little-endian: low byte = lo register (F, C, E, L), high byte = hi (A, B, D, H).
    union { uint16_t AF; struct { Flags  F; uint8_t A; }; };
    union { uint16_t BC; struct { uint8_t C; uint8_t B; }; };
    union { uint16_t DE; struct { uint8_t E; uint8_t D; }; };
    union { uint16_t HL; struct { uint8_t L; uint8_t H; }; };

    // Shadow registers (swapped by EX AF,AF' and EXX)
    uint16_t AF_ = 0, BC_ = 0, DE_ = 0, HL_ = 0;

    // Index registers
    union { uint16_t IX; struct { uint8_t IXL; uint8_t IXH; }; };
    union { uint16_t IY; struct { uint8_t IYL; uint8_t IYH; }; };

    // Special registers
    uint16_t SP = 0, PC = 0;
    uint8_t  I  = 0, R  = 0;        // Interrupt vector, Refresh counter
    bool     IFF1 = false, IFF2 = false;
    uint8_t  IM     = 0;             // Interrupt mode (0, 1, or 2)
    bool     halted = false;
};

// ── Z80 CPU ──────────────────────────────────────────────────────────────────
class Z80 {
public:
    explicit Z80(Bus& bus);

    void reset();          // PC=0, SP=0xFFFF, AF=0xFFFF, others 0, IFF=false, IM=0
    int  step();           // Execute one instruction; returns T-states consumed
    void nmi();            // Trigger Non-Maskable Interrupt
    void irq();            // Trigger maskable IRQ (only if IFF1 is set)

    const Z80Registers& getRegisters() const;

private:
    Z80Registers regs;
    Bus&         bus;

    // ── Memory access ──────────────────────────────────────────────────────
    uint8_t  read8(uint16_t addr);
    void     write8(uint16_t addr, uint8_t val);
    uint16_t read16(uint16_t addr);
    void     write16(uint16_t addr, uint16_t val);
    uint8_t  fetch8();     // read8(PC++)
    uint16_t fetch16();    // read16(PC); PC += 2

    // ── I/O ────────────────────────────────────────────────────────────────
    uint8_t ioRead(uint8_t port);
    void    ioWrite(uint8_t port, uint8_t val);

    // ── Flag helpers ───────────────────────────────────────────────────────
    void setFlagsINC(uint8_t prev, uint8_t result);
    void setFlagsDEC(uint8_t prev, uint8_t result);
    void setFlagsAdd8(uint8_t a, uint8_t b, bool carry = false);
    void setFlagsSub8(uint8_t a, uint8_t b, bool carry = false);
    void setFlagsAnd(uint8_t result);
    void setFlagsOr(uint8_t result);
    void setFlagsXor(uint8_t result);
    void setFlagsCp(uint8_t a, uint8_t b);
    bool parity(uint8_t val);  // true if even number of set bits

    // ── Opcode table executors (called from step()) ─────────────────────────
    int executeMain(uint8_t opcode);
    int executeCB(uint8_t opcode);
    int executeED(uint8_t opcode);
    int executeDD(uint8_t opcode);   // IX-prefixed instructions
    int executeFD(uint8_t opcode);   // IY-prefixed instructions
    int executeIndexed(uint8_t opcode, uint16_t& idx);       // shared DD/FD handler
    int executeIndexedCB(int8_t d, uint8_t op, uint16_t& idx); // DDCB/FDCB handler

    // ── Interrupt helpers ──────────────────────────────────────────────────
    void pushPC();
    void handleNMI();
    void handleIRQ();
};
