// src/cpu/z80_opcodes.cpp
#include "cpu/z80.h"

#include <cstdio>
#include <cstdint>

int Z80::executeMain(uint8_t opcode) {

    // ── Register index helpers (0=B 1=C 2=D 3=E 4=H 5=L 6=(HL) 7=A) ──────────
    auto getReg = [&](uint8_t idx) -> uint8_t {
        switch (idx) {
            case 0: return regs.B;
            case 1: return regs.C;
            case 2: return regs.D;
            case 3: return regs.E;
            case 4: return regs.H;
            case 5: return regs.L;
            case 6: return read8(regs.HL);
            case 7: return regs.A;
            default: return 0;
        }
    };

    auto setReg = [&](uint8_t idx, uint8_t val) {
        switch (idx) {
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

    // ── LD r,r' block (0x40–0x7F) ────────────────────────────────────────────
    if (opcode >= 0x40 && opcode <= 0x7F) {
        if (opcode == 0x76) {           // HALT
            regs.halted = true;
            return 4;
        }
        uint8_t dst = (opcode >> 3) & 7;
        uint8_t src = opcode & 7;
        setReg(dst, getReg(src));
        return (src == 6 || dst == 6) ? 7 : 4;
    }

    // ── ALU r block (0x80–0xBF): ADD/ADC/SUB/SBC/AND/XOR/OR/CP A,r ──────────
    if (opcode >= 0x80 && opcode <= 0xBF) {
        uint8_t aluOp = (opcode >> 3) & 7;
        uint8_t src   = opcode & 7;
        uint8_t val   = getReg(src);
        int     cy    = (src == 6) ? 7 : 4;
        switch (aluOp) {
            case 0: setFlagsAdd8(regs.A, val);                                    break; // ADD
            case 1: { bool c = regs.F.C != 0; setFlagsAdd8(regs.A, val, c);      break; } // ADC
            case 2: setFlagsSub8(regs.A, val);                                    break; // SUB
            case 3: { bool c = regs.F.C != 0; setFlagsSub8(regs.A, val, c);      break; } // SBC
            case 4: regs.A &= val; setFlagsAnd(regs.A);                           break; // AND
            case 5: regs.A ^= val; setFlagsXor(regs.A);                           break; // XOR
            case 6: regs.A |= val; setFlagsOr(regs.A);                            break; // OR
            case 7: setFlagsCp(regs.A, val);                                       break; // CP
            default: break;
        }
        return cy;
    }

    switch (opcode) {

        // ── GROUP 1: NOP and 16-bit loads ─────────────────────────────────────
        case 0x00: return 4;                                                   // NOP

        case 0x01: regs.BC = fetch16(); return 10;                             // LD BC,nn
        case 0x11: regs.DE = fetch16(); return 10;                             // LD DE,nn
        case 0x21: regs.HL = fetch16(); return 10;                             // LD HL,nn
        case 0x31: regs.SP = fetch16(); return 10;                             // LD SP,nn

        case 0x02: write8(regs.BC, regs.A); return 7;                         // LD (BC),A
        case 0x12: write8(regs.DE, regs.A); return 7;                         // LD (DE),A
        case 0x0A: regs.A = read8(regs.BC); return 7;                         // LD A,(BC)
        case 0x1A: regs.A = read8(regs.DE); return 7;                         // LD A,(DE)

        case 0x22: { uint16_t a = fetch16(); write16(a, regs.HL); return 20; } // LD (nn),HL
        case 0x2A: { uint16_t a = fetch16(); regs.HL = read16(a); return 20; } // LD HL,(nn)
        case 0x32: { uint16_t a = fetch16(); write8(a, regs.A);   return 13; } // LD (nn),A
        case 0x3A: { uint16_t a = fetch16(); regs.A = read8(a);   return 13; } // LD A,(nn)

        case 0xF9: regs.SP = regs.HL; return 6;                               // LD SP,HL

        // ── GROUP 2: INC/DEC 16-bit ────────────────────────────────────────────
        case 0x03: ++regs.BC; return 6;
        case 0x13: ++regs.DE; return 6;
        case 0x23: ++regs.HL; return 6;
        case 0x33: ++regs.SP; return 6;
        case 0x0B: --regs.BC; return 6;
        case 0x1B: --regs.DE; return 6;
        case 0x2B: --regs.HL; return 6;
        case 0x3B: --regs.SP; return 6;

        // ── GROUP 3: INC 8-bit ─────────────────────────────────────────────────
        case 0x04: { uint8_t p = regs.B; ++regs.B; setFlagsINC(p, regs.B); return 4; }
        case 0x0C: { uint8_t p = regs.C; ++regs.C; setFlagsINC(p, regs.C); return 4; }
        case 0x14: { uint8_t p = regs.D; ++regs.D; setFlagsINC(p, regs.D); return 4; }
        case 0x1C: { uint8_t p = regs.E; ++regs.E; setFlagsINC(p, regs.E); return 4; }
        case 0x24: { uint8_t p = regs.H; ++regs.H; setFlagsINC(p, regs.H); return 4; }
        case 0x2C: { uint8_t p = regs.L; ++regs.L; setFlagsINC(p, regs.L); return 4; }
        case 0x3C: { uint8_t p = regs.A; ++regs.A; setFlagsINC(p, regs.A); return 4; }
        case 0x34: {                                                           // INC (HL)
            uint8_t tmp = read8(regs.HL);
            uint8_t p   = tmp;
            ++tmp;
            setFlagsINC(p, tmp);
            write8(regs.HL, tmp);
            return 11;
        }

        // ── GROUP 3: DEC 8-bit ─────────────────────────────────────────────────
        case 0x05: { uint8_t p = regs.B; --regs.B; setFlagsDEC(p, regs.B); return 4; }
        case 0x0D: { uint8_t p = regs.C; --regs.C; setFlagsDEC(p, regs.C); return 4; }
        case 0x15: { uint8_t p = regs.D; --regs.D; setFlagsDEC(p, regs.D); return 4; }
        case 0x1D: { uint8_t p = regs.E; --regs.E; setFlagsDEC(p, regs.E); return 4; }
        case 0x25: { uint8_t p = regs.H; --regs.H; setFlagsDEC(p, regs.H); return 4; }
        case 0x2D: { uint8_t p = regs.L; --regs.L; setFlagsDEC(p, regs.L); return 4; }
        case 0x3D: { uint8_t p = regs.A; --regs.A; setFlagsDEC(p, regs.A); return 4; }
        case 0x35: {                                                           // DEC (HL)
            uint8_t tmp = read8(regs.HL);
            uint8_t p   = tmp;
            --tmp;
            setFlagsDEC(p, tmp);
            write8(regs.HL, tmp);
            return 11;
        }

        // ── GROUP 4: LD r,n ────────────────────────────────────────────────────
        case 0x06: regs.B = fetch8(); return 7;
        case 0x0E: regs.C = fetch8(); return 7;
        case 0x16: regs.D = fetch8(); return 7;
        case 0x1E: regs.E = fetch8(); return 7;
        case 0x26: regs.H = fetch8(); return 7;
        case 0x2E: regs.L = fetch8(); return 7;
        case 0x3E: regs.A = fetch8(); return 7;
        case 0x36: write8(regs.HL, fetch8()); return 10;                       // LD (HL),n

        // ── GROUP 7: ALU A,n (immediate) ──────────────────────────────────────
        case 0xC6: setFlagsAdd8(regs.A, fetch8()); return 7;                   // ADD A,n
        case 0xCE: { bool c = regs.F.C != 0; uint8_t n = fetch8(); setFlagsAdd8(regs.A, n, c); return 7; } // ADC A,n
        case 0xD6: setFlagsSub8(regs.A, fetch8()); return 7;                   // SUB n
        case 0xDE: { bool c = regs.F.C != 0; uint8_t n = fetch8(); setFlagsSub8(regs.A, n, c); return 7; } // SBC A,n
        case 0xE6: { regs.A &= fetch8(); setFlagsAnd(regs.A); return 7; }      // AND n
        case 0xEE: { regs.A ^= fetch8(); setFlagsXor(regs.A); return 7; }      // XOR n
        case 0xF6: { regs.A |= fetch8(); setFlagsOr(regs.A);  return 7; }      // OR  n
        case 0xFE: setFlagsCp(regs.A, fetch8()); return 7;                     // CP  n

        // ── GROUP 8: Jumps ─────────────────────────────────────────────────────
        case 0xC3: regs.PC = fetch16(); return 10;                             // JP nn
        case 0xE9: regs.PC = regs.HL;  return 4;                              // JP (HL)

        case 0xC2: { uint16_t nn = fetch16(); if (!regs.F.Z)  regs.PC = nn; return 10; } // JP NZ
        case 0xCA: { uint16_t nn = fetch16(); if ( regs.F.Z)  regs.PC = nn; return 10; } // JP Z
        case 0xD2: { uint16_t nn = fetch16(); if (!regs.F.C)  regs.PC = nn; return 10; } // JP NC
        case 0xDA: { uint16_t nn = fetch16(); if ( regs.F.C)  regs.PC = nn; return 10; } // JP C
        case 0xE2: { uint16_t nn = fetch16(); if (!regs.F.PV) regs.PC = nn; return 10; } // JP PO
        case 0xEA: { uint16_t nn = fetch16(); if ( regs.F.PV) regs.PC = nn; return 10; } // JP PE
        case 0xF2: { uint16_t nn = fetch16(); if (!regs.F.S)  regs.PC = nn; return 10; } // JP P
        case 0xFA: { uint16_t nn = fetch16(); if ( regs.F.S)  regs.PC = nn; return 10; } // JP M

        case 0x18: {                                                           // JR e
            int8_t e = static_cast<int8_t>(fetch8());
            regs.PC  = static_cast<uint16_t>(regs.PC + e);
            return 12;
        }
        case 0x20: { int8_t e = static_cast<int8_t>(fetch8()); if (!regs.F.Z) { regs.PC = static_cast<uint16_t>(regs.PC + e); return 12; } return 7; } // JR NZ
        case 0x28: { int8_t e = static_cast<int8_t>(fetch8()); if ( regs.F.Z) { regs.PC = static_cast<uint16_t>(regs.PC + e); return 12; } return 7; } // JR Z
        case 0x30: { int8_t e = static_cast<int8_t>(fetch8()); if (!regs.F.C) { regs.PC = static_cast<uint16_t>(regs.PC + e); return 12; } return 7; } // JR NC
        case 0x38: { int8_t e = static_cast<int8_t>(fetch8()); if ( regs.F.C) { regs.PC = static_cast<uint16_t>(regs.PC + e); return 12; } return 7; } // JR C

        case 0x10: {                                                           // DJNZ e
            int8_t e = static_cast<int8_t>(fetch8());
            if (--regs.B != 0) {
                regs.PC = static_cast<uint16_t>(regs.PC + e);
                return 13;
            }
            return 8;
        }

        // ── GROUP 9: Calls ─────────────────────────────────────────────────────
        case 0xCD: { uint16_t nn = fetch16(); pushPC(); regs.PC = nn; return 17; } // CALL nn

        case 0xC4: { uint16_t nn = fetch16(); if (!regs.F.Z)  { pushPC(); regs.PC = nn; return 17; } return 10; } // CALL NZ
        case 0xCC: { uint16_t nn = fetch16(); if ( regs.F.Z)  { pushPC(); regs.PC = nn; return 17; } return 10; } // CALL Z
        case 0xD4: { uint16_t nn = fetch16(); if (!regs.F.C)  { pushPC(); regs.PC = nn; return 17; } return 10; } // CALL NC
        case 0xDC: { uint16_t nn = fetch16(); if ( regs.F.C)  { pushPC(); regs.PC = nn; return 17; } return 10; } // CALL C
        case 0xE4: { uint16_t nn = fetch16(); if (!regs.F.PV) { pushPC(); regs.PC = nn; return 17; } return 10; } // CALL PO
        case 0xEC: { uint16_t nn = fetch16(); if ( regs.F.PV) { pushPC(); regs.PC = nn; return 17; } return 10; } // CALL PE
        case 0xF4: { uint16_t nn = fetch16(); if (!regs.F.S)  { pushPC(); regs.PC = nn; return 17; } return 10; } // CALL P
        case 0xFC: { uint16_t nn = fetch16(); if ( regs.F.S)  { pushPC(); regs.PC = nn; return 17; } return 10; } // CALL M

        // ── GROUP 9: Returns ───────────────────────────────────────────────────
        case 0xC9: { regs.PC = read16(regs.SP); regs.SP += 2; return 10; }    // RET

        case 0xC0: { if (!regs.F.Z)  { regs.PC = read16(regs.SP); regs.SP += 2; return 11; } return 5; } // RET NZ
        case 0xC8: { if ( regs.F.Z)  { regs.PC = read16(regs.SP); regs.SP += 2; return 11; } return 5; } // RET Z
        case 0xD0: { if (!regs.F.C)  { regs.PC = read16(regs.SP); regs.SP += 2; return 11; } return 5; } // RET NC
        case 0xD8: { if ( regs.F.C)  { regs.PC = read16(regs.SP); regs.SP += 2; return 11; } return 5; } // RET C
        case 0xE0: { if (!regs.F.PV) { regs.PC = read16(regs.SP); regs.SP += 2; return 11; } return 5; } // RET PO
        case 0xE8: { if ( regs.F.PV) { regs.PC = read16(regs.SP); regs.SP += 2; return 11; } return 5; } // RET PE
        case 0xF0: { if (!regs.F.S)  { regs.PC = read16(regs.SP); regs.SP += 2; return 11; } return 5; } // RET P
        case 0xF8: { if ( regs.F.S)  { regs.PC = read16(regs.SP); regs.SP += 2; return 11; } return 5; } // RET M

        // ── GROUP 10: Stack ────────────────────────────────────────────────────
        case 0xC5: regs.SP -= 2; write16(regs.SP, regs.BC); return 11;        // PUSH BC
        case 0xD5: regs.SP -= 2; write16(regs.SP, regs.DE); return 11;        // PUSH DE
        case 0xE5: regs.SP -= 2; write16(regs.SP, regs.HL); return 11;        // PUSH HL
        case 0xF5: regs.SP -= 2; write16(regs.SP, regs.AF); return 11;        // PUSH AF

        case 0xC1: regs.BC = read16(regs.SP); regs.SP += 2; return 10;        // POP BC
        case 0xD1: regs.DE = read16(regs.SP); regs.SP += 2; return 10;        // POP DE
        case 0xE1: regs.HL = read16(regs.SP); regs.SP += 2; return 10;        // POP HL
        case 0xF1: regs.AF = read16(regs.SP); regs.SP += 2; return 10;        // POP AF

        // ── GROUP 11: RST ──────────────────────────────────────────────────────
        case 0xC7: pushPC(); regs.PC = 0x0000; return 11;
        case 0xCF: pushPC(); regs.PC = 0x0008; return 11;
        case 0xD7: pushPC(); regs.PC = 0x0010; return 11;
        case 0xDF: pushPC(); regs.PC = 0x0018; return 11;
        case 0xE7: pushPC(); regs.PC = 0x0020; return 11;
        case 0xEF: pushPC(); regs.PC = 0x0028; return 11;
        case 0xF7: pushPC(); regs.PC = 0x0030; return 11;
        case 0xFF: pushPC(); regs.PC = 0x0038; return 11;

        // ── GROUP 12: Misc ─────────────────────────────────────────────────────
        case 0x07: {                                                            // RLCA
            uint8_t b7 = static_cast<uint8_t>((regs.A >> 7) & 1u);
            regs.A    = static_cast<uint8_t>((regs.A << 1) | b7);
            regs.F.C  = b7;
            regs.F.N  = 0;
            regs.F.H  = 0;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            return 4;
        }
        case 0x0F: {                                                            // RRCA
            uint8_t b0 = regs.A & 1u;
            regs.A    = static_cast<uint8_t>((regs.A >> 1) | (b0 << 7));
            regs.F.C  = b0;
            regs.F.N  = 0;
            regs.F.H  = 0;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            return 4;
        }
        case 0x17: {                                                            // RLA
            uint8_t oldC = regs.F.C;
            regs.F.C  = static_cast<uint8_t>((regs.A >> 7) & 1u);
            regs.A    = static_cast<uint8_t>((regs.A << 1) | oldC);
            regs.F.N  = 0;
            regs.F.H  = 0;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            return 4;
        }
        case 0x1F: {                                                            // RRA
            uint8_t oldC = regs.F.C;
            regs.F.C  = regs.A & 1u;
            regs.A    = static_cast<uint8_t>((regs.A >> 1) | (oldC << 7));
            regs.F.N  = 0;
            regs.F.H  = 0;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            return 4;
        }
        case 0x2F: {                                                            // CPL
            regs.A    = ~regs.A;
            regs.F.H  = 1;
            regs.F.N  = 1;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            return 4;
        }
        case 0x3F: {                                                            // CCF
            regs.F.H  = regs.F.C;
            regs.F.C  = !regs.F.C;
            regs.F.N  = 0;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            return 4;
        }
        case 0x37: {                                                            // SCF
            regs.F.C  = 1;
            regs.F.H  = 0;
            regs.F.N  = 0;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            return 4;
        }
        case 0x27: {                                                            // DAA
            uint8_t a    = regs.A;
            bool    n    = regs.F.N  != 0;
            bool    c    = regs.F.C  != 0;
            bool    h    = regs.F.H  != 0;
            uint8_t diff = 0;
            bool    newC = c;

            if (!n) {
                if (h || (a & 0x0Fu) > 9u) diff |= 0x06u;
                if (c || a > 0x99u)        { diff |= 0x60u; newC = true; }
                a = static_cast<uint8_t>(a + diff);
            } else {
                if (h || (a & 0x0Fu) > 9u) diff |= 0x06u;
                if (c || a > 0x99u)        { diff |= 0x60u; newC = true; }
                a = static_cast<uint8_t>(a - diff);
            }

            regs.F.S  = (a & 0x80u) != 0;
            regs.F.Z  = (a == 0);
            regs.F.H  = !n ? ((diff & 0x06u) && (regs.A & 0x0Fu) > 9u)   // carry from bit 3 when lo+6 overflows nibble
                           : (h && (regs.A & 0x0Fu) < 6u);              // borrow when lo-6 underflows nibble
            regs.F.PV = parity(a);
            regs.F.C  = newC;
            // N unchanged
            regs.F.F5 = (a & 0x20u) != 0;
            regs.F.F3 = (a & 0x08u) != 0;
            regs.A    = a;
            return 4;
        }

        case 0xF3: regs.IFF1 = regs.IFF2 = false; return 4;                   // DI
        case 0xFB: regs.IFF1 = regs.IFF2 = true;  return 4;                   // EI

        case 0xEB: {                                                            // EX DE,HL
            uint16_t tmp = regs.DE;
            regs.DE = regs.HL;
            regs.HL = tmp;
            return 4;
        }
        case 0x08: {                                                            // EX AF,AF'
            uint16_t tmp = regs.AF;
            regs.AF  = regs.AF_;
            regs.AF_ = tmp;
            return 4;
        }
        case 0xD9: {                                                            // EXX
            uint16_t tmp;
            tmp = regs.BC; regs.BC = regs.BC_; regs.BC_ = tmp;
            tmp = regs.DE; regs.DE = regs.DE_; regs.DE_ = tmp;
            tmp = regs.HL; regs.HL = regs.HL_; regs.HL_ = tmp;
            return 4;
        }
        case 0xE3: {                                                            // EX (SP),HL
            uint16_t tmp = read16(regs.SP);
            write16(regs.SP, regs.HL);
            regs.HL = tmp;
            return 19;
        }

        default:
            std::fprintf(stderr,
                         "Z80: unhandled main opcode 0x%02X at PC=0x%04X\n",
                         opcode,
                         static_cast<uint16_t>(regs.PC - 1u));
            return 4;
    }
}
