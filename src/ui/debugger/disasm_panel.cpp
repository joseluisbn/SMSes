// src/ui/debugger/disasm_panel.cpp
#include "ui/debugger/disasm_panel.h"
#include "cpu/z80.h"
#include "memory/bus.h"
#include <imgui.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

// ── Helpers ────────────────────────────────────────────────────────────────

static const char* REG8[8]  = { "B","C","D","E","H","L","(HL)","A" };
static const char* REG16[4] = { "BC","DE","HL","SP" };
static const char* REG16Q[4]= { "BC","DE","HL","AF" };   // for PUSH/POP
static const char* CC[8]    = { "NZ","Z","NC","C","PO","PE","P","M" };

static std::string hex8(uint8_t v)
{
    char buf[7]; std::snprintf(buf, sizeof(buf), "0x%02X", v); return buf;
}
static std::string hex16(uint16_t v)
{
    char buf[9]; std::snprintf(buf, sizeof(buf), "0x%04X", v); return buf;
}
static std::string rel(int8_t d, uint16_t pc)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%04X (%+d)",
                  static_cast<uint16_t>(pc + d), d);
    return buf;
}

// ── Constructor ────────────────────────────────────────────────────────────

DisasmPanel::DisasmPanel(Z80& cpu, Bus& bus)
    : cpu(cpu), bus(bus)
{}

bool DisasmPanel::isVisible() const  { return visible; }
void DisasmPanel::setVisible(bool v) { visible = v; }

// ── Disassembler ───────────────────────────────────────────────────────────

std::pair<std::string, int> DisasmPanel::disassemble(uint16_t addr)
{
    auto rd = [&](uint16_t a) -> uint8_t { return bus.read(a); };
    uint8_t op = rd(addr);
    int     len = 1;

    // ── CB prefix ────────────────────────────────────────────────────────
    if (op == 0xCB) {
        uint8_t cb = rd(addr + 1); len = 2;
        const char* r = REG8[cb & 7];
        int bit = (cb >> 3) & 7;
        if      (cb < 0x08) return { "RLC "  + std::string(r), len };
        else if (cb < 0x10) return { "RRC "  + std::string(r), len };
        else if (cb < 0x18) return { "RL "   + std::string(r), len };
        else if (cb < 0x20) return { "RR "   + std::string(r), len };
        else if (cb < 0x28) return { "SLA "  + std::string(r), len };
        else if (cb < 0x30) return { "SRA "  + std::string(r), len };
        else if (cb < 0x38) return { "SLL "  + std::string(r), len };
        else if (cb < 0x40) return { "SRL "  + std::string(r), len };
        else if (cb < 0x80) return { "BIT "  + std::to_string(bit) + "," + r, len };
        else if (cb < 0xC0) return { "RES "  + std::to_string(bit) + "," + r, len };
        else                return { "SET "  + std::to_string(bit) + "," + r, len };
    }

    // ── ED prefix ────────────────────────────────────────────────────────
    if (op == 0xED) {
        uint8_t ed = rd(addr + 1); len = 2;
        switch (ed) {
            case 0x44: return { "NEG",  len };
            case 0x45: return { "RETN", len };
            case 0x4D: return { "RETI", len };
            case 0x46: return { "IM 0", len };
            case 0x56: return { "IM 1", len };
            case 0x5E: return { "IM 2", len };
            case 0x47: return { "LD I,A", len };
            case 0x4F: return { "LD R,A", len };
            case 0x57: return { "LD A,I", len };
            case 0x5F: return { "LD A,R", len };
            case 0xA0: return { "LDI",  len };
            case 0xA1: return { "CPI",  len };
            case 0xA8: return { "LDD",  len };
            case 0xA9: return { "CPD",  len };
            case 0xB0: return { "LDIR", len };
            case 0xB1: return { "CPIR", len };
            case 0xB8: return { "LDDR", len };
            case 0xB9: return { "CPDR", len };
            default: {
                // LD rr,(nn) / LD (nn),rr / ADC/SBC HL,rr
                int rr = (ed >> 4) & 3;
                if ((ed & 0x0F) == 0x02) {
                    uint16_t nn = rd(addr+2) | (rd(addr+3) << 8); len = 4;
                    return { std::string("LD (") + hex16(nn) + ")," + REG16[rr], len };
                }
                if ((ed & 0x0F) == 0x0A) {
                    uint16_t nn = rd(addr+2) | (rd(addr+3) << 8); len = 4;
                    return { std::string("LD ") + REG16[rr] + ",(" + hex16(nn) + ")", len };
                }
                if ((ed & 0x07) == 0x02 && (ed & 0xC0) == 0x40) {
                    const char* op2 = (ed & 0x08) ? "SBC" : "ADC";
                    return { std::string(op2) + " HL," + REG16[rr], len };
                }
                char buf[16]; std::snprintf(buf, sizeof(buf), "DB 0x%02X,0x%02X", op, ed);
                return { buf, len };
            }
        }
    }

    // ── DD / FD prefix (IX / IY) ──────────────────────────────────────────
    if (op == 0xDD || op == 0xFD) {
        const char* idx = (op == 0xDD) ? "IX" : "IY";
        uint8_t     sub = rd(addr + 1); len = 2;

        if (sub == 0xE9) return { std::string("JP (") + idx + ")", len };
        if (sub == 0xF9) return { std::string("LD SP,")  + idx, len };
        if (sub == 0xE5) return { std::string("PUSH ") + idx, len };
        if (sub == 0xE1) return { std::string("POP ")  + idx, len };
        if (sub == 0x21) {
            uint16_t nn = rd(addr+2) | (rd(addr+3) << 8); len = 4;
            return { std::string("LD ") + idx + "," + hex16(nn), len };
        }
        if (sub == 0x22) {
            uint16_t nn = rd(addr+2) | (rd(addr+3) << 8); len = 4;
            return { std::string("LD (") + hex16(nn) + ")," + idx, len };
        }
        if (sub == 0x2A) {
            uint16_t nn = rd(addr+2) | (rd(addr+3) << 8); len = 4;
            return { std::string("LD ") + idx + ",(" + hex16(nn) + ")", len };
        }
        if (sub == 0x36) {
            int8_t  d  = static_cast<int8_t>(rd(addr+2));
            uint8_t n  = rd(addr+3); len = 4;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "LD (%s%+d),%s", idx, d, hex8(n).c_str());
            return { buf, len };
        }
        // LD r,(IX+d)
        if ((sub & 0xC7) == 0x46 && (sub & 0x38) != 0x30) {
            int8_t d = static_cast<int8_t>(rd(addr+2)); len = 3;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "LD %s,(%s%+d)", REG8[(sub>>3)&7], idx, d);
            return { buf, len };
        }
        // LD (IX+d),r
        if ((sub & 0xF8) == 0x70 && sub != 0x76) {
            int8_t d = static_cast<int8_t>(rd(addr+2)); len = 3;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "LD (%s%+d),%s", idx, d, REG8[sub&7]);
            return { buf, len };
        }
        // ADD/ADC/SUB/SBC/AND/XOR/OR/CP (IX+d)
        if (sub >= 0x80 && sub <= 0xBF && (sub & 7) == 6) {
            static const char* alu[8] = { "ADD A,","ADC A,","SUB ","SBC A,",
                                           "AND ","XOR ","OR ","CP " };
            int8_t d = static_cast<int8_t>(rd(addr+2)); len = 3;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%s(%s%+d)", alu[(sub>>3)&7], idx, d);
            return { buf, len };
        }
        // ADD IX,rr
        if ((sub & 0xCF) == 0x09) {
            return { std::string("ADD ") + idx + "," + REG16[(sub>>4)&3], len };
        }
        char buf[24]; std::snprintf(buf, sizeof(buf), "DB 0x%02X,0x%02X", op, sub);
        return { buf, len };
    }

    // ── Unprefixed opcodes ────────────────────────────────────────────────
    switch (op) {
        case 0x00: return { "NOP", 1 };
        case 0x07: return { "RLCA", 1 };
        case 0x0F: return { "RRCA", 1 };
        case 0x17: return { "RLA",  1 };
        case 0x1F: return { "RRA",  1 };
        case 0x27: return { "DAA",  1 };
        case 0x2F: return { "CPL",  1 };
        case 0x37: return { "SCF",  1 };
        case 0x3F: return { "CCF",  1 };
        case 0x76: return { "HALT", 1 };
        case 0xF3: return { "DI",   1 };
        case 0xFB: return { "EI",   1 };
        case 0xEB: return { "EX DE,HL", 1 };
        case 0x08: return { "EX AF,AF'", 1 };
        case 0xD9: return { "EXX", 1 };
        case 0xE3: return { "EX (SP),HL", 1 };
        case 0xE9: return { "JP (HL)", 1 };
        case 0xF9: return { "LD SP,HL", 1 };
        case 0xC9: return { "RET", 1 };
        case 0xED: return { "DB 0xED", 1 }; // handled above but satisfy clang

        // DJNZ / JR
        case 0x10: {
            int8_t d = static_cast<int8_t>(rd(addr+1));
            return { "DJNZ " + rel(d, static_cast<uint16_t>(addr + 2)), 2 };
        }
        case 0x18: {
            int8_t d = static_cast<int8_t>(rd(addr+1));
            return { "JR " + rel(d, static_cast<uint16_t>(addr + 2)), 2 };
        }
        case 0x20: case 0x28: case 0x30: case 0x38: {
            int8_t d = static_cast<int8_t>(rd(addr+1));
            int cc = (op - 0x20) >> 3;
            return { "JR " + std::string(CC[cc]) + "," +
                     rel(d, static_cast<uint16_t>(addr + 2)), 2 };
        }

        // LD rr, nn
        case 0x01: case 0x11: case 0x21: case 0x31: {
            uint16_t nn = rd(addr+1) | (rd(addr+2) << 8);
            return { std::string("LD ") + REG16[(op>>4)&3] + "," + hex16(nn), 3 };
        }

        // INC / DEC rr
        case 0x03: case 0x13: case 0x23: case 0x33:
            return { std::string("INC ") + REG16[(op>>4)&3], 1 };
        case 0x0B: case 0x1B: case 0x2B: case 0x3B:
            return { std::string("DEC ") + REG16[(op>>4)&3], 1 };

        // ADD HL, rr
        case 0x09: case 0x19: case 0x29: case 0x39:
            return { std::string("ADD HL,") + REG16[(op>>4)&3], 1 };

        // INC r / DEC r
        case 0x04: case 0x0C: case 0x14: case 0x1C:
        case 0x24: case 0x2C: case 0x34: case 0x3C:
            return { std::string("INC ") + REG8[(op>>3)&7], 1 };
        case 0x05: case 0x0D: case 0x15: case 0x1D:
        case 0x25: case 0x2D: case 0x35: case 0x3D:
            return { std::string("DEC ") + REG8[(op>>3)&7], 1 };

        // LD r, n
        case 0x06: case 0x0E: case 0x16: case 0x1E:
        case 0x26: case 0x2E: case 0x36: case 0x3E:
            return { std::string("LD ") + REG8[(op>>3)&7] + "," + hex8(rd(addr+1)), 2 };

        // LD (BC/DE),A / LD A,(BC/DE)
        case 0x02: return { "LD (BC),A", 1 };
        case 0x0A: return { "LD A,(BC)", 1 };
        case 0x12: return { "LD (DE),A", 1 };
        case 0x1A: return { "LD A,(DE)", 1 };

        // LD (nn),HL / LD HL,(nn) / LD (nn),A / LD A,(nn)
        case 0x22: { uint16_t nn = rd(addr+1)|(rd(addr+2)<<8);
                     return { "LD (" + hex16(nn) + "),HL", 3 }; }
        case 0x2A: { uint16_t nn = rd(addr+1)|(rd(addr+2)<<8);
                     return { "LD HL,(" + hex16(nn) + ")", 3 }; }
        case 0x32: { uint16_t nn = rd(addr+1)|(rd(addr+2)<<8);
                     return { "LD (" + hex16(nn) + "),A", 3 }; }
        case 0x3A: { uint16_t nn = rd(addr+1)|(rd(addr+2)<<8);
                     return { "LD A,(" + hex16(nn) + ")", 3 }; }

        // PUSH / POP rr
        case 0xC1: case 0xD1: case 0xE1: case 0xF1:
            return { std::string("POP ")  + REG16Q[(op>>4)&3], 1 };
        case 0xC5: case 0xD5: case 0xE5: case 0xF5:
            return { std::string("PUSH ") + REG16Q[(op>>4)&3], 1 };

        // JP nn / JP cc,nn
        case 0xC3: { uint16_t nn = rd(addr+1)|(rd(addr+2)<<8);
                     return { "JP " + hex16(nn), 3 }; }
        case 0xC2: case 0xCA: case 0xD2: case 0xDA:
        case 0xE2: case 0xEA: case 0xF2: case 0xFA: {
            uint16_t nn = rd(addr+1)|(rd(addr+2)<<8);
            return { "JP " + std::string(CC[(op>>3)&7]) + "," + hex16(nn), 3 };
        }

        // CALL nn / CALL cc,nn
        case 0xCD: { uint16_t nn = rd(addr+1)|(rd(addr+2)<<8);
                     return { "CALL " + hex16(nn), 3 }; }
        case 0xC4: case 0xCC: case 0xD4: case 0xDC:
        case 0xE4: case 0xEC: case 0xF4: case 0xFC: {
            uint16_t nn = rd(addr+1)|(rd(addr+2)<<8);
            return { "CALL " + std::string(CC[(op>>3)&7]) + "," + hex16(nn), 3 };
        }

        // RET cc
        case 0xC0: case 0xC8: case 0xD0: case 0xD8:
        case 0xE0: case 0xE8: case 0xF0: case 0xF8:
            return { "RET " + std::string(CC[(op>>3)&7]), 1 };

        // RST n
        case 0xC7: case 0xCF: case 0xD7: case 0xDF:
        case 0xE7: case 0xEF: case 0xF7: case 0xFF: {
            char buf[16]; std::snprintf(buf, sizeof(buf), "RST 0x%02X", op & 0x38);
            return { buf, 1 };
        }

        // ALU A, n (immediate)
        case 0xC6: return { "ADD A," + hex8(rd(addr+1)), 2 };
        case 0xCE: return { "ADC A," + hex8(rd(addr+1)), 2 };
        case 0xD6: return { "SUB "   + hex8(rd(addr+1)), 2 };
        case 0xDE: return { "SBC A," + hex8(rd(addr+1)), 2 };
        case 0xE6: return { "AND "   + hex8(rd(addr+1)), 2 };
        case 0xEE: return { "XOR "   + hex8(rd(addr+1)), 2 };
        case 0xF6: return { "OR "    + hex8(rd(addr+1)), 2 };
        case 0xFE: return { "CP "    + hex8(rd(addr+1)), 2 };

        // IN A,(n) / OUT (n),A
        case 0xDB: return { "IN A,("  + hex8(rd(addr+1)) + ")", 2 };
        case 0xD3: return { "OUT ("   + hex8(rd(addr+1)) + "),A", 2 };

        default:
            break;
    }

    // LD r,r (0x40–0x7F, excluding HALT 0x76)
    if (op >= 0x40 && op <= 0x7F) {
        return { std::string("LD ") + REG8[(op>>3)&7] + "," + REG8[op&7], 1 };
    }

    // ALU A,r (0x80–0xBF)
    if (op >= 0x80 && op <= 0xBF) {
        static const char* alu[8] = { "ADD A,","ADC A,","SUB ","SBC A,",
                                       "AND ","XOR ","OR ","CP " };
        return { std::string(alu[(op>>3)&7]) + REG8[op&7], 1 };
    }

    char buf[12]; std::snprintf(buf, sizeof(buf), "DB 0x%02X", op);
    return { buf, 1 };
}

// ── Draw helpers ───────────────────────────────────────────────────────────

void DisasmPanel::drawDisassembly()
{
    const uint16_t currentPC = cpu.getRegisters().PC;
    uint16_t       addr      = followPC ? currentPC : currentPC;

    ImGui::BeginChild("##disasm", ImVec2(0, 300), false);

    for (int i = 0; i < disasmLines; ++i) {
        auto [mnemonic, len] = disassemble(addr);

        const bool isCurrent = (addr == currentPC);
        const bool isBp = std::any_of(breakpoints.begin(), breakpoints.end(),
                                      [addr](const Breakpoint& b) {
                                          return b.enabled && b.addr == addr;
                                      });

        if (isCurrent)
            ImGui::TextColored({0.2f, 1.0f, 0.2f, 1.0f},
                               "\xe2\x86\x92 %04X  %s", addr, mnemonic.c_str());
        else if (isBp)
            ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f},
                               "\xe2\x97\x8f %04X  %s", addr, mnemonic.c_str());
        else
            ImGui::Text("  %04X  %s", addr, mnemonic.c_str());

        addr = static_cast<uint16_t>(addr + len);
    }

    ImGui::EndChild();
}

void DisasmPanel::drawBreakpoints()
{
    ImGui::Text("Breakpoints:");

    for (int i = 0; i < static_cast<int>(breakpoints.size()); ++i) {
        Breakpoint& bp = breakpoints[i];
        ImGui::PushID(i);

        ImGui::Checkbox("##en", &bp.enabled);
        ImGui::SameLine();
        ImGui::Text("%04X", bp.addr);
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            breakpoints.erase(breakpoints.begin() + i);
            ImGui::PopID();
            break;   // iterator invalidated — restart next frame
        }

        ImGui::PopID();
    }
}

// ── Public API ─────────────────────────────────────────────────────────────

bool DisasmPanel::shouldBreak() const
{
    const uint16_t pc = cpu.getRegisters().PC;
    return std::any_of(breakpoints.begin(), breakpoints.end(),
                       [pc](const Breakpoint& b) {
                           return b.enabled && b.addr == pc;
                       });
}

void DisasmPanel::draw()
{
    if (!visible) return;

    ImGui::Begin("Disassembler", &visible);

    ImGui::Checkbox("Follow PC", &followPC);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputText("BP Addr", bpAddrBuf, sizeof(bpAddrBuf),
                     ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    if (ImGui::Button("Add BP")) {
        const uint16_t a = static_cast<uint16_t>(
            std::strtoul(bpAddrBuf, nullptr, 16));
        breakpoints.push_back({ a, true });
    }

    ImGui::Separator();
    drawDisassembly();
    ImGui::Separator();
    drawBreakpoints();

    ImGui::End();
}
