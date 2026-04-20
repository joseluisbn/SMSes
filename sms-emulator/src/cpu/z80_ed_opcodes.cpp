// src/cpu/z80_ed_opcodes.cpp
#include "cpu/z80.h"

#include <cstdio>
#include <cstdint>

int Z80::executeED(uint8_t opcode) {

    // ── Register index helpers (0=B 1=C 2=D 3=E 4=H 5=L 6=F/(HL) 7=A) ───────
    auto getReg = [&](uint8_t idx) -> uint8_t {
        switch (idx) {
            case 0: return regs.B;
            case 1: return regs.C;
            case 2: return regs.D;
            case 3: return regs.E;
            case 4: return regs.H;
            case 5: return regs.L;
            case 6: return regs.F.raw;
            case 7: return regs.A;
            default: return 0u;
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
            case 6: regs.F.raw = val; break;
            case 7: regs.A = val; break;
            default: break;
        }
    };

    // ── 16-bit ADC helper ─────────────────────────────────────────────────────
    auto adcHL = [&](uint16_t rr) {
        uint32_t hl  = regs.HL;
        uint32_t r   = hl + rr + static_cast<uint32_t>(regs.F.C);
        regs.F.S  = (r & 0x8000u) != 0;
        regs.F.Z  = (r & 0xFFFFu) == 0;
        regs.F.H  = ((hl ^ rr ^ r) & 0x1000u) != 0;
        regs.F.PV = ((~(hl ^ rr) & (hl ^ r)) & 0x8000u) != 0;
        regs.F.N  = 0;
        regs.F.C  = (r & 0x10000u) != 0;
        regs.F.F5 = (r & 0x2000u) != 0;   // bit 13 = bit 5 of high byte
        regs.F.F3 = (r & 0x0800u) != 0;   // bit 11 = bit 3 of high byte
        regs.HL   = static_cast<uint16_t>(r);
    };

    // ── 16-bit SBC helper ─────────────────────────────────────────────────────
    auto sbcHL = [&](uint16_t rr) {
        uint32_t hl  = regs.HL;
        uint32_t r   = hl - rr - static_cast<uint32_t>(regs.F.C);
        regs.F.S  = (r & 0x8000u) != 0;
        regs.F.Z  = (r & 0xFFFFu) == 0;
        regs.F.H  = ((hl ^ rr ^ r) & 0x1000u) != 0;
        regs.F.PV = (((hl ^ rr) & (hl ^ r)) & 0x8000u) != 0;
        regs.F.N  = 1;
        regs.F.C  = (r & 0x10000u) != 0;
        regs.F.F5 = (r & 0x2000u) != 0;
        regs.F.F3 = (r & 0x0800u) != 0;
        regs.HL   = static_cast<uint16_t>(r);
    };

    // ── LDI core (returns updated BC) ────────────────────────────────────────
    auto doLDI = [&]() {
        uint8_t val = read8(regs.HL);
        write8(regs.DE, val);
        ++regs.HL;
        ++regs.DE;
        --regs.BC;
        uint8_t n  = static_cast<uint8_t>(regs.A + val);
        regs.F.PV = (regs.BC != 0);
        regs.F.H  = 0;
        regs.F.N  = 0;
        regs.F.F5 = (n & 0x02u) != 0;   // Z80 quirk: bit 1 of (A+(HL))
        regs.F.F3 = (n & 0x08u) != 0;
    };

    // ── LDD core ─────────────────────────────────────────────────────────────
    auto doLDD = [&]() {
        uint8_t val = read8(regs.HL);
        write8(regs.DE, val);
        --regs.HL;
        --regs.DE;
        --regs.BC;
        uint8_t n  = static_cast<uint8_t>(regs.A + val);
        regs.F.PV = (regs.BC != 0);
        regs.F.H  = 0;
        regs.F.N  = 0;
        regs.F.F5 = (n & 0x02u) != 0;
        regs.F.F3 = (n & 0x08u) != 0;
    };

    // ── CPI core ─────────────────────────────────────────────────────────────
    auto doCPI = [&]() {
        uint8_t val = read8(regs.HL);
        ++regs.HL;
        --regs.BC;
        uint8_t r  = static_cast<uint8_t>(regs.A - val);
        bool    h  = (static_cast<int>(regs.A & 0x0Fu) - static_cast<int>(val & 0x0Fu)) < 0;
        uint8_t n  = static_cast<uint8_t>(r - static_cast<uint8_t>(h));
        regs.F.S  = (r & 0x80u) != 0;
        regs.F.Z  = (r == 0u);
        regs.F.H  = h;
        regs.F.PV = (regs.BC != 0);
        regs.F.N  = 1;
        // C unchanged
        regs.F.F5 = (n & 0x02u) != 0;   // Z80 quirk: bit 1 of (A - val - H)
        regs.F.F3 = (n & 0x08u) != 0;
    };

    // ── CPD core ─────────────────────────────────────────────────────────────
    auto doCPD = [&]() {
        uint8_t val = read8(regs.HL);
        --regs.HL;
        --regs.BC;
        uint8_t r  = static_cast<uint8_t>(regs.A - val);
        bool    h  = (static_cast<int>(regs.A & 0x0Fu) - static_cast<int>(val & 0x0Fu)) < 0;
        uint8_t n  = static_cast<uint8_t>(r - static_cast<uint8_t>(h));
        regs.F.S  = (r & 0x80u) != 0;
        regs.F.Z  = (r == 0u);
        regs.F.H  = h;
        regs.F.PV = (regs.BC != 0);
        regs.F.N  = 1;
        // C unchanged
        regs.F.F5 = (n & 0x02u) != 0;
        regs.F.F3 = (n & 0x08u) != 0;
    };

    // ── IN r,(C) pattern ─────────────────────────────────────────────────────
    // Opcodes 0x40/0x48/0x50/0x58/0x60/0x68/0x70/0x78 — bits 5-3 = reg index
    // 0x70 = IN F,(C): undocumented — sets flags, discards value, does NOT write to F
    if ((opcode & 0xC7u) == 0x40u) {
        uint8_t idx = (opcode >> 3u) & 7u;
        uint8_t val = ioRead(regs.C);
        if (idx != 6u) setReg(idx, val);   // idx==6 is IN F,(C): result discarded
        regs.F.S  = (val & 0x80u) != 0;
        regs.F.Z  = (val == 0u);
        regs.F.H  = 0;
        regs.F.PV = parity(val);
        regs.F.N  = 0;
        regs.F.F5 = (val & 0x20u) != 0;
        regs.F.F3 = (val & 0x08u) != 0;
        // C unchanged
        return 12;
    }

    // ── OUT (C),r pattern ─────────────────────────────────────────────────────
    // Opcodes 0x41/0x49/0x51/0x59/0x61/0x69/0x71/0x79 — bits 5-3 = reg index
    if ((opcode & 0xC7u) == 0x41u) {
        uint8_t idx = (opcode >> 3u) & 7u;
        uint8_t val = (idx == 6u) ? 0x00u : getReg(idx);  // 0x71 outputs 0
        ioWrite(regs.C, val);
        return 12;
    }

    switch (opcode) {

        // ── BLOCK TRANSFERS ──────────────────────────────────────────────────
        case 0xA0: doLDI(); return 16;                                         // LDI
        case 0xA8: doLDD(); return 16;                                         // LDD
        case 0xB0:                                                             // LDIR
            doLDI();
            if (regs.BC != 0) { regs.PC -= 2; return 21; }
            return 16;
        case 0xB8:                                                             // LDDR
            doLDD();
            if (regs.BC != 0) { regs.PC -= 2; return 21; }
            return 16;

        // ── BLOCK SEARCH ─────────────────────────────────────────────────────
        case 0xA1: doCPI(); return 16;                                         // CPI
        case 0xA9: doCPD(); return 16;                                         // CPD
        case 0xB1:                                                             // CPIR
            doCPI();
            if (regs.BC != 0 && !regs.F.Z) { regs.PC -= 2; return 21; }
            return 16;
        case 0xB9:                                                             // CPDR
            doCPD();
            if (regs.BC != 0 && !regs.F.Z) { regs.PC -= 2; return 21; }
            return 16;

        // ── 16-BIT SBC ───────────────────────────────────────────────────────
        case 0x42: sbcHL(regs.BC); return 15;
        case 0x52: sbcHL(regs.DE); return 15;
        case 0x62: sbcHL(regs.HL); return 15;
        case 0x72: sbcHL(regs.SP); return 15;

        // ── 16-BIT ADC ───────────────────────────────────────────────────────
        case 0x4A: adcHL(regs.BC); return 15;
        case 0x5A: adcHL(regs.DE); return 15;
        case 0x6A: adcHL(regs.HL); return 15;
        case 0x7A: adcHL(regs.SP); return 15;

        // ── 16-BIT LOADS ─────────────────────────────────────────────────────
        case 0x43: { uint16_t a = fetch16(); write16(a, regs.BC); return 20; }
        case 0x53: { uint16_t a = fetch16(); write16(a, regs.DE); return 20; }
        case 0x63: { uint16_t a = fetch16(); write16(a, regs.HL); return 20; }
        case 0x73: { uint16_t a = fetch16(); write16(a, regs.SP); return 20; }
        case 0x4B: { uint16_t a = fetch16(); regs.BC = read16(a); return 20; }
        case 0x5B: { uint16_t a = fetch16(); regs.DE = read16(a); return 20; }
        case 0x6B: { uint16_t a = fetch16(); regs.HL = read16(a); return 20; }
        case 0x7B: { uint16_t a = fetch16(); regs.SP = read16(a); return 20; }

        // ── INTERRUPT MODE ───────────────────────────────────────────────────
        case 0x46: regs.IM = 0; return 8;
        case 0x56: regs.IM = 1; return 8;
        case 0x5E: regs.IM = 2; return 8;

        // ── INTERRUPT VECTOR REGISTER LOADS ─────────────────────────────────
        case 0x47: regs.I = regs.A; return 9;                                 // LD I,A
        case 0x4F: regs.R = regs.A; return 9;                                 // LD R,A

        case 0x57: {                                                           // LD A,I
            regs.A    = regs.I;
            regs.F.S  = (regs.A & 0x80u) != 0;
            regs.F.Z  = (regs.A == 0u);
            regs.F.H  = 0;
            regs.F.N  = 0;
            regs.F.PV = regs.IFF2;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            // C unchanged
            return 9;
        }
        case 0x5F: {                                                           // LD A,R
            regs.A    = regs.R;
            regs.F.S  = (regs.A & 0x80u) != 0;
            regs.F.Z  = (regs.A == 0u);
            regs.F.H  = 0;
            regs.F.N  = 0;
            regs.F.PV = regs.IFF2;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            // C unchanged
            return 9;
        }

        // ── RETN / RETI ─────────────────────────────────────────────────────
        case 0x45:                                                             // RETN
        case 0x4D: {                                                           // RETI
            regs.IFF1 = regs.IFF2;
            regs.PC   = read16(regs.SP);
            regs.SP  += 2;
            return 14;
        }

        // ── NEG ─────────────────────────────────────────────────────────────
        case 0x44: {                                                           // NEG
            uint8_t tmp = regs.A;
            regs.A      = 0u;
            setFlagsSub8(0u, tmp);
            return 8;
        }

        // ── I/O BLOCK ────────────────────────────────────────────────────────
        case 0xA2: {                                                           // INI
            write8(regs.HL, ioRead(regs.C));
            ++regs.HL;
            --regs.B;
            regs.F.Z = (regs.B == 0u);
            regs.F.N = 1;
            return 16;
        }
        case 0xAA: {                                                           // IND
            write8(regs.HL, ioRead(regs.C));
            --regs.HL;
            --regs.B;
            regs.F.Z = (regs.B == 0u);
            regs.F.N = 1;
            return 16;
        }
        case 0xB2:                                                             // INIR
            write8(regs.HL, ioRead(regs.C));
            ++regs.HL;
            --regs.B;
            regs.F.Z = (regs.B == 0u);
            regs.F.N = 1;
            if (regs.B != 0u) { regs.PC -= 2; return 21; }
            return 16;
        case 0xBA:                                                             // INDR
            write8(regs.HL, ioRead(regs.C));
            --regs.HL;
            --regs.B;
            regs.F.Z = (regs.B == 0u);
            regs.F.N = 1;
            if (regs.B != 0u) { regs.PC -= 2; return 21; }
            return 16;

        case 0xA3: {                                                           // OUTI
            --regs.B;
            ioWrite(regs.C, read8(regs.HL));
            ++regs.HL;
            regs.F.Z = (regs.B == 0u);
            regs.F.N = 1;
            return 16;
        }
        case 0xAB: {                                                           // OUTD
            --regs.B;
            ioWrite(regs.C, read8(regs.HL));
            --regs.HL;
            regs.F.Z = (regs.B == 0u);
            regs.F.N = 1;
            return 16;
        }
        case 0xB3:                                                             // OTIR
            --regs.B;
            ioWrite(regs.C, read8(regs.HL));
            ++regs.HL;
            regs.F.Z = (regs.B == 0u);
            regs.F.N = 1;
            if (regs.B != 0u) { regs.PC -= 2; return 21; }
            return 16;
        case 0xBB:                                                             // OTDR
            --regs.B;
            ioWrite(regs.C, read8(regs.HL));
            --regs.HL;
            regs.F.Z = (regs.B == 0u);
            regs.F.N = 1;
            if (regs.B != 0u) { regs.PC -= 2; return 21; }
            return 16;

        // ── ROTATE DIGIT ────────────────────────────────────────────────────
        case 0x6F: {                                                           // RLD
            uint8_t m  = read8(regs.HL);
            uint8_t newM = static_cast<uint8_t>((m << 4u) | (regs.A & 0x0Fu));
            regs.A       = static_cast<uint8_t>((regs.A & 0xF0u) | (m >> 4u));
            write8(regs.HL, newM);
            regs.F.S  = (regs.A & 0x80u) != 0;
            regs.F.Z  = (regs.A == 0u);
            regs.F.H  = 0;
            regs.F.PV = parity(regs.A);
            regs.F.N  = 0;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            // C unchanged
            return 18;
        }
        case 0x67: {                                                           // RRD
            uint8_t m  = read8(regs.HL);
            uint8_t newM = static_cast<uint8_t>(((regs.A & 0x0Fu) << 4u) | (m >> 4u));
            regs.A       = static_cast<uint8_t>((regs.A & 0xF0u) | (m & 0x0Fu));
            write8(regs.HL, newM);
            regs.F.S  = (regs.A & 0x80u) != 0;
            regs.F.Z  = (regs.A == 0u);
            regs.F.H  = 0;
            regs.F.PV = parity(regs.A);
            regs.F.N  = 0;
            regs.F.F5 = (regs.A & 0x20u) != 0;
            regs.F.F3 = (regs.A & 0x08u) != 0;
            // C unchanged
            return 18;
        }

        default:
            std::fprintf(stderr,
                         "Z80: unhandled ED opcode 0x%02X at PC=0x%04X\n",
                         opcode,
                         static_cast<uint16_t>(regs.PC - 1u));
            return 8;
    }
}
