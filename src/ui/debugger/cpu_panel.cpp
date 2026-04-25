// src/ui/debugger/cpu_panel.cpp
#include "ui/debugger/cpu_panel.h"
#include "cpu/z80.h"
#include <imgui.h>

CpuPanel::CpuPanel(Z80& cpu)
    : cpu(cpu)
{}

bool CpuPanel::isVisible()   const { return visible; }
void CpuPanel::setVisible(bool v)  { visible = v; }
bool CpuPanel::isStepMode()  const { return stepMode; }
bool CpuPanel::popStep()           { bool s = doStep; doStep = false; return s; }

void CpuPanel::drawRegisters()
{
    const auto& r = cpu.getRegisters();

    ImGui::Columns(2, "regs", false);

    ImGui::Text("AF  = %04X   A=%02X",           r.AF, r.A);
    ImGui::Text("BC  = %04X   B=%02X C=%02X",    r.BC, r.B, r.C);
    ImGui::Text("DE  = %04X   D=%02X E=%02X",    r.DE, r.D, r.E);
    ImGui::Text("HL  = %04X   H=%02X L=%02X",    r.HL, r.H, r.L);

    ImGui::NextColumn();

    ImGui::Text("AF' = %04X", r.AF_);
    ImGui::Text("BC' = %04X", r.BC_);
    ImGui::Text("DE' = %04X", r.DE_);
    ImGui::Text("HL' = %04X", r.HL_);

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::Text("IX = %04X   IY = %04X", r.IX, r.IY);
    ImGui::Text("SP = %04X   PC = %04X", r.SP, r.PC);
    ImGui::Text("I  = %02X     R  = %02X",  r.I, r.R);
    ImGui::Text("IM = %d   IFF1=%d IFF2=%d",
                r.IM, r.IFF1 ? 1 : 0, r.IFF2 ? 1 : 0);
    ImGui::Text("HALTED: %s", r.halted ? "YES" : "no");
}

void CpuPanel::drawFlags()
{
    const auto& r = cpu.getRegisters();
    const uint8_t f = r.F.raw;

    ImGui::Text("Flags: S=%d Z=%d H=%d PV=%d N=%d C=%d",
                (f >> 7) & 1, (f >> 6) & 1, (f >> 4) & 1,
                (f >> 2) & 1, (f >> 1) & 1,  f & 1);

    auto flagLight = [&](const char* name, bool on) {
        const ImVec4 col = on ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f)
                              : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        ImGui::TextColored(col, "%s", name);
        ImGui::SameLine();
    };

    flagLight("S",  (f >> 7) & 1);
    flagLight("Z",  (f >> 6) & 1);
    flagLight("H",  (f >> 4) & 1);
    flagLight("PV", (f >> 2) & 1);
    flagLight("N",  (f >> 1) & 1);
    flagLight("C",   f & 1);
    ImGui::NewLine();
}

void CpuPanel::drawControls()
{
    ImGui::Checkbox("Step mode", &stepMode);
    if (stepMode) {
        ImGui::SameLine();
        if (ImGui::Button("Step"))
            doStep = true;
        ImGui::SameLine();
        ImGui::TextDisabled("(one instruction per click)");
    }
}

void CpuPanel::draw()
{
    if (!visible) return;

    ImGui::Begin("CPU Registers", &visible);

    drawControls();
    ImGui::Separator();
    drawRegisters();
    ImGui::Separator();
    drawFlags();

    ImGui::End();
}
