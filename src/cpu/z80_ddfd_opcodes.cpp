// src/cpu/z80_ddfd_opcodes.cpp
#include "cpu/z80.h"

#include <cstdio>
#include <cstdint>

// ── DD/FD CB (indexed bit ops) ────────────────────────────────────────────────
int Z80::executeIndexedCB(int8_t d, uint8_t op, uint16_t& IDX) {
    const uint16_t addr   = static_cast<uint16_t>(IDX + d);
    const uint8_t  opType = (op >> 6u) & 3u;  // 00=rot/shift  01=BIT  10=RES  11=SET
    const uint8_t  par    = (op >> 3u) & 7u;  // rotation type or bit index
    const uint8_t  regIdx = op & 7u;           // result also stored here (if != 6)
    uint8_t        val    = read8(addr);

    // Helper: write back to memory and optionally to encoded register (undocumented)
    auto commit = [&](uint8_t result) {
        write8(addr, result);
        if (regIdx != 6u) {
            switch (regIdx) {
                case 0: regs.B = result; break;
                case 1: regs.C = result; break;
                case 2: regs.D = result; break;
                case 3: regs.E = result; break;
                case 4: regs.H = result; break;
                case 5: regs.L = result; break;
                case 7: regs.A = result; break;
                default: break;
            }
        }
    };

    // ── 00: Rotations / Shifts ────────────────────────────────────────────────
    if (opType == 0u) {
        uint8_t result = 0u;
        switch (par) {
            case 0u: { uint8_t b7 = static_cast<uint8_t>((val >> 7u) & 1u); result = static_cast<uint8_t>((val << 1u) | b7); regs.F.C = b7; break; }  // RLC
            case 1u: { uint8_t b0 = val & 1u; result = static_cast<uint8_t>((val >> 1u) | (b0 << 7u)); regs.F.C = b0; break; }                         // RRC
            case 2u: { uint8_t oC = regs.F.C; regs.F.C = static_cast<uint8_t>((val >> 7u) & 1u); result = static_cast<uint8_t>((val << 1u) | oC); break; } // RL
            case 3u: { uint8_t oC = regs.F.C; regs.F.C = val & 1u; result = static_cast<uint8_t>((val >> 1u) | (oC << 7u)); break; }                   // RR
            case 4u: { regs.F.C = static_cast<uint8_t>((val >> 7u) & 1u); result = static_cast<uint8_t>(val << 1u); break; }                            // SLA
            case 5u: { regs.F.C = val & 1u; result = static_cast<uint8_t>((val >> 1u) | (val & 0x80u)); break; }                                        // SRA
            case 6u: { regs.F.C = static_cast<uint8_t>((val >> 7u) & 1u); result = static_cast<uint8_t>((val << 1u) | 1u); break; }                    // SLL (undoc)
            case 7u: { regs.F.C = val & 1u; result = static_cast<uint8_t>(val >> 1u); break; }                                                          // SRL
            default: break;
        }
        commit(result);
        regs.F.S  = (result & 0x80u) != 0;
        regs.F.Z  = (result == 0u);
        regs.F.H  = 0;
        regs.F.PV = parity(result);
        regs.F.N  = 0;
        regs.F.F5 = (result & 0x20u) != 0;
        regs.F.F3 = (result & 0x08u) != 0;
        return 23;
    }

    // ── 01: BIT (no writeback) ────────────────────────────────────────────────
    if (opType == 1u) {
        bool bitSet   = (val & (1u << par)) != 0u;
        regs.F.Z  = !bitSet;
        regs.F.H  = 1;
        regs.F.N  = 0;
        regs.F.PV = !bitSet;
        regs.F.S  = (par == 7u) && bitSet;
        // C unchanged
        // F5/F3 from high byte of address (undocumented Z80 behaviour for BIT (IX+d))
        regs.F.F5 = (static_cast<uint8_t>(addr >> 8u) & 0x20u) != 0;
        regs.F.F3 = (static_cast<uint8_t>(addr >> 8u) & 0x08u) != 0;
        return 20;
    }

    // ── 10: RES ───────────────────────────────────────────────────────────────
    if (opType == 2u) {
        uint8_t result = val & static_cast<uint8_t>(~(1u << par));
        commit(result);
        return 23;
    }

    // ── 11: SET ───────────────────────────────────────────────────────────────
    {
        uint8_t result = val | static_cast<uint8_t>(1u << par);
        commit(result);
        return 23;
    }
}

// ── Shared DD/FD implementation ───────────────────────────────────────────────
int Z80::executeIndexed(uint8_t opcode, uint16_t& IDX) {

    // ── LD r,(IDX+d): (opcode & 0xC7) == 0x46, excl. 0x76 ───────────────────
    if ((opcode & 0xC7u) == 0x46u && opcode != 0x76u) {
        uint8_t  dst  = (opcode >> 3u) & 7u;
        int8_t   d    = static_cast<int8_t>(fetch8());
        uint16_t addr = static_cast<uint16_t>(IDX + d);
        uint8_t  val  = read8(addr);
        switch (dst) {
            case 0: regs.B = val; break;
            case 1: regs.C = val; break;
            case 2: regs.D = val; break;
            case 3: regs.E = val; break;
            case 4: regs.H = val; break;
            case 5: regs.L = val; break;
            case 7: regs.A = val; break;
            default: break;
        }
        return 19;
    }

    // ── LD (IDX+d),r: opcode 0x70-0x77 excl. 0x76 ────────────────────────────
    if ((opcode & 0xF8u) == 0x70u && opcode != 0x76u) {
        uint8_t  src  = opcode & 7u;
        int8_t   d    = static_cast<int8_t>(fetch8());
        uint16_t addr = static_cast<uint16_t>(IDX + d);
        uint8_t  val  = 0u;
        switch (src) {
            case 0: val = regs.B; break;
            case 1: val = regs.C; break;
            case 2: val = regs.D; break;
            case 3: val = regs.E; break;
            case 4: val = regs.H; break;
            case 5: val = regs.L; break;
            case 7: val = regs.A; break;
            default: break;
        }
        write8(addr, val);
        return 19;
    }

    // ── ALU A,(IDX+d): (opcode & 0xC7) == 0x86 ───────────────────────────────
    if ((opcode & 0xC7u) == 0x86u) {
        uint8_t  aluOp = (opcode >> 3u) & 7u;
        int8_t   d     = static_cast<int8_t>(fetch8());
        uint8_t  val   = read8(static_cast<uint16_t>(IDX + d));
        switch (aluOp) {
            case 0: setFlagsAdd8(regs.A, val);                                         break; // ADD
            case 1: { bool c = regs.F.C != 0; setFlagsAdd8(regs.A, val, c);            break; } // ADC
            case 2: setFlagsSub8(regs.A, val);                                         break; // SUB
            case 3: { bool c = regs.F.C != 0; setFlagsSub8(regs.A, val, c);            break; } // SBC
            case 4: regs.A &= val; setFlagsAnd(regs.A);                                break; // AND
            case 5: regs.A ^= val; setFlagsXor(regs.A);                                break; // XOR
            case 6: regs.A |= val; setFlagsOr(regs.A);                                 break; // OR
            case 7: setFlagsCp(regs.A, val);                                            break; // CP
            default: break;
        }
        return 19;
    }

    switch (opcode) {

        // ── Load / Store ───────────────────────────────────────────────────────
        case 0x21: IDX = fetch16(); return 14;                                 // LD IDX,nn
        case 0x22: { uint16_t a = fetch16(); write16(a, IDX); return 20; }    // LD (nn),IDX
        case 0x2A: { uint16_t a = fetch16(); IDX = read16(a); return 20; }    // LD IDX,(nn)
        case 0xF9: regs.SP = IDX; return 10;                                  // LD SP,IDX

        // ── INC / DEC IDX ──────────────────────────────────────────────────────
        case 0x23: ++IDX; return 10;
        case 0x2B: --IDX; return 10;

        // ── INC/DEC (IDX+d) ───────────────────────────────────────────────────
        case 0x34: {                                                            // INC (IDX+d)
            int8_t   d    = static_cast<int8_t>(fetch8());
            uint16_t addr = static_cast<uint16_t>(IDX + d);
            uint8_t  tmp  = read8(addr);
            uint8_t  prev = tmp++;
            setFlagsINC(prev, tmp);
            write8(addr, tmp);
            return 23;
        }
        case 0x35: {                                                            // DEC (IDX+d)
            int8_t   d    = static_cast<int8_t>(fetch8());
            uint16_t addr = static_cast<uint16_t>(IDX + d);
            uint8_t  tmp  = read8(addr);
            uint8_t  prev = tmp--;
            setFlagsDEC(prev, tmp);
            write8(addr, tmp);
            return 23;
        }
        case 0x36: {                                                            // LD (IDX+d),n
            int8_t   d    = static_cast<int8_t>(fetch8());
            uint8_t  n    = fetch8();
            write8(static_cast<uint16_t>(IDX + d), n);
            return 19;
        }

        // ── ADD IDX,rr ────────────────────────────────────────────────────────
        case 0x09: {
            uint32_t r = IDX + regs.BC;
            regs.F.H   = ((IDX ^ regs.BC ^ r) & 0x1000u) != 0;
            regs.F.N   = 0;
            regs.F.C   = (r & 0x10000u) != 0;
            regs.F.F5  = (r & 0x2000u) != 0;
            regs.F.F3  = (r & 0x0800u) != 0;
            IDX        = static_cast<uint16_t>(r);
            return 15;
        }
        case 0x19: {
            uint32_t r = IDX + regs.DE;
            regs.F.H   = ((IDX ^ regs.DE ^ r) & 0x1000u) != 0;
            regs.F.N   = 0;
            regs.F.C   = (r & 0x10000u) != 0;
            regs.F.F5  = (r & 0x2000u) != 0;
            regs.F.F3  = (r & 0x0800u) != 0;
            IDX        = static_cast<uint16_t>(r);
            return 15;
        }
        case 0x29: {
            uint16_t snap = IDX;                                               // ADD IDX,IDX
            uint32_t r    = static_cast<uint32_t>(snap) + snap;
            regs.F.H   = ((snap ^ snap ^ r) & 0x1000u) != 0;
            regs.F.N   = 0;
            regs.F.C   = (r & 0x10000u) != 0;
            regs.F.F5  = (r & 0x2000u) != 0;
            regs.F.F3  = (r & 0x0800u) != 0;
            IDX        = static_cast<uint16_t>(r);
            return 15;
        }
        case 0x39: {
            uint32_t r = IDX + regs.SP;
            regs.F.H   = ((IDX ^ regs.SP ^ r) & 0x1000u) != 0;
            regs.F.N   = 0;
            regs.F.C   = (r & 0x10000u) != 0;
            regs.F.F5  = (r & 0x2000u) != 0;
            regs.F.F3  = (r & 0x0800u) != 0;
            IDX        = static_cast<uint16_t>(r);
            return 15;
        }

        // ── Stack ──────────────────────────────────────────────────────────────
        case 0xE5: regs.SP -= 2; write16(regs.SP, IDX); return 15;            // PUSH IDX
        case 0xE1: IDX = read16(regs.SP); regs.SP += 2; return 14;            // POP IDX
        case 0xE3: {                                                            // EX (SP),IDX
            uint16_t tmp = read16(regs.SP);
            write16(regs.SP, IDX);
            IDX = tmp;
            return 23;
        }

        // ── Jump ──────────────────────────────────────────────────────────────
        case 0xE9: regs.PC = IDX; return 8;                                    // JP (IDX)

        // ── HALT ──────────────────────────────────────────────────────────────
        case 0x76: regs.halted = true; return 4;

        // ── DDCB / FDCB ───────────────────────────────────────────────────────
        case 0xCB: {
            int8_t  d  = static_cast<int8_t>(fetch8());
            uint8_t cb = fetch8();
            return executeIndexedCB(d, cb, IDX);
        }

        // ── Fall through to main opcode table for non-IDX opcodes ─────────────
        default:
            return executeMain(opcode);
    }
}

// ── Public entry points ───────────────────────────────────────────────────────
int Z80::executeDD(uint8_t opcode) { return executeIndexed(opcode, regs.IX); }
int Z80::executeFD(uint8_t opcode) { return executeIndexed(opcode, regs.IY); }
// Stubs in z80.cpp are superseded by these definitions above.
// executeIndexed and executeIndexedCB are declared in z80.h and implemented here.
