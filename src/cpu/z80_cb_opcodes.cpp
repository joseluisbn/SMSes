// src/cpu/z80_cb_opcodes.cpp
#include "cpu/z80.h"

#include <cstdint>

int Z80::executeCB(uint8_t opcode) {
    const uint8_t op  = (opcode >> 6) & 3u;  // 00=rot/shift  01=BIT  10=RES  11=SET
    const uint8_t par = (opcode >> 3) & 7u;  // rotation type or bit index
    const uint8_t reg = opcode & 7u;          // register index (6 = (HL))
    const bool    isHL = (reg == 6);

    // ── Register accessors ────────────────────────────────────────────────────
    auto getReg = [&]() -> uint8_t {
        switch (reg) {
            case 0: return regs.B;
            case 1: return regs.C;
            case 2: return regs.D;
            case 3: return regs.E;
            case 4: return regs.H;
            case 5: return regs.L;
            case 6: return read8(regs.HL);
            case 7: return regs.A;
            default: return 0u;
        }
    };

    auto setReg = [&](uint8_t val) {
        switch (reg) {
            case 0: regs.B = val; break;
            case 1: regs.C = val; break;
            case 2: regs.D = val; break;
            case 3: regs.E = val; break;
            case 4: regs.H = val; break;
            case 5: regs.L = val; break;
            case 6: write8(regs.HL, val); break;
            case 7: regs.A = val; break;
            default: break;
        }
    };

    // ── 00: Rotations / Shifts ────────────────────────────────────────────────
    if (op == 0u) {
        uint8_t v      = getReg();
        uint8_t result = 0u;

        switch (par) {
            case 0u: {   // RLC — rotate left circular
                uint8_t b7 = static_cast<uint8_t>((v >> 7u) & 1u);
                result     = static_cast<uint8_t>((v << 1u) | b7);
                regs.F.C   = b7;
                break;
            }
            case 1u: {   // RRC — rotate right circular
                uint8_t b0 = v & 1u;
                result     = static_cast<uint8_t>((v >> 1u) | (b0 << 7u));
                regs.F.C   = b0;
                break;
            }
            case 2u: {   // RL — rotate left through carry
                uint8_t oldC = regs.F.C;
                regs.F.C     = static_cast<uint8_t>((v >> 7u) & 1u);
                result       = static_cast<uint8_t>((v << 1u) | oldC);
                break;
            }
            case 3u: {   // RR — rotate right through carry
                uint8_t oldC = regs.F.C;
                regs.F.C     = v & 1u;
                result       = static_cast<uint8_t>((v >> 1u) | (oldC << 7u));
                break;
            }
            case 4u: {   // SLA — shift left arithmetic
                regs.F.C = static_cast<uint8_t>((v >> 7u) & 1u);
                result   = static_cast<uint8_t>(v << 1u);
                break;
            }
            case 5u: {   // SRA — shift right arithmetic (preserves sign bit)
                regs.F.C = v & 1u;
                result   = static_cast<uint8_t>((v >> 1u) | (v & 0x80u));
                break;
            }
            case 6u: {   // SLL — shift left logical, undocumented (bit 0 set to 1)
                regs.F.C = static_cast<uint8_t>((v >> 7u) & 1u);
                result   = static_cast<uint8_t>((v << 1u) | 1u);
                break;
            }
            case 7u: {   // SRL — shift right logical
                regs.F.C = v & 1u;
                result   = static_cast<uint8_t>(v >> 1u);
                break;
            }
            default: break;
        }

        setReg(result);

        regs.F.S  = (result & 0x80u) != 0;
        regs.F.Z  = (result == 0u);
        regs.F.H  = 0;
        regs.F.PV = parity(result);
        regs.F.N  = 0;
        regs.F.F5 = (result & 0x20u) != 0;
        regs.F.F3 = (result & 0x08u) != 0;

        return isHL ? 15 : 8;
    }

    // ── 01: BIT ───────────────────────────────────────────────────────────────
    if (op == 1u) {
        uint8_t v      = getReg();
        bool    bitSet = (v & (1u << par)) != 0u;

        regs.F.Z  = !bitSet;
        regs.F.H  = 1;
        regs.F.N  = 0;
        regs.F.PV = !bitSet;                        // PV mirrors Z
        regs.F.S  = (par == 7u) && bitSet;
        // C unchanged
        // F5/F3: from value for registers; from H (addr high byte) for (HL)
        uint8_t f53src = isHL ? regs.H : v;
        regs.F.F5 = (f53src & 0x20u) != 0;
        regs.F.F3 = (f53src & 0x08u) != 0;

        return isHL ? 12 : 8;
    }

    // ── 10: RES ───────────────────────────────────────────────────────────────
    if (op == 2u) {
        uint8_t v = getReg();
        v &= static_cast<uint8_t>(~(1u << par));
        setReg(v);
        return isHL ? 15 : 8;
    }

    // ── 11: SET ───────────────────────────────────────────────────────────────
    {
        uint8_t v = getReg();
        v |= static_cast<uint8_t>(1u << par);
        setReg(v);
        return isHL ? 15 : 8;
    }
}
