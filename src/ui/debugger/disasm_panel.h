// src/ui/debugger/disasm_panel.h
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class Z80;
class Bus;

struct Breakpoint {
    uint16_t addr;
    bool     enabled = true;
};

class DisasmPanel {
public:
    DisasmPanel(Z80& cpu, Bus& bus);

    void draw();
    bool isVisible()    const;
    void setVisible(bool v);

    bool shouldBreak()  const;

private:
    Z80& cpu;
    Bus& bus;
    bool visible = false;

    std::vector<Breakpoint> breakpoints;
    char  bpAddrBuf[5] = "0000";
    bool  followPC     = true;
    int   disasmLines  = 20;

    std::pair<std::string, int> disassemble(uint16_t addr);

    void drawDisassembly();
    void drawBreakpoints();
};
