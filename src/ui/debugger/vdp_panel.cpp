// src/ui/debugger/vdp_panel.cpp
#include "ui/debugger/vdp_panel.h"
#include <imgui.h>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

VdpPanel::VdpPanel(VDP& vdp) : vdp(vdp) {}

// ─────────────────────────────────────────────────────────────────────────────
// Visibility
// ─────────────────────────────────────────────────────────────────────────────

bool VdpPanel::isVisible() const { return visible; }
void VdpPanel::setVisible(bool v) { visible = v; }

// ─────────────────────────────────────────────────────────────────────────────
// draw()
// ─────────────────────────────────────────────────────────────────────────────

void VdpPanel::draw()
{
    if (!visible) return;

    ImGui::Begin("VDP Viewer", &visible);

    ImGui::Checkbox("Registers", &showRegisters);
    ImGui::SameLine();
    ImGui::Checkbox("Palette", &showPalette);
    ImGui::SameLine();
    ImGui::Checkbox("Sprites", &showSprites);
    ImGui::Separator();

    if (showRegisters) drawRegisters();
    if (showPalette)   drawPalette();
    if (showSprites)   drawSprites();

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
// drawRegisters() — R0–R10 with decoded field names
// ─────────────────────────────────────────────────────────────────────────────

void VdpPanel::drawRegisters()
{
    const uint8_t* r = vdp.getRegs();

    ImGui::Text("R0:  %02X  mode ctrl 1  | line IRQ: %s | sprites shift: %s",
        r[0],
        (r[0] & 0x10) ? "ON" : "OFF",
        (r[0] & 0x08) ? "ON" : "OFF");

    ImGui::Text("R1:  %02X  mode ctrl 2  | display: %s | frame IRQ: %s | size: %s",
        r[1],
        (r[1] & 0x40) ? "ON"  : "OFF",
        (r[1] & 0x20) ? "ON"  : "OFF",
        (r[1] & 0x02) ? "8x16" : "8x8");

    ImGui::Text("R2:  %02X  name table   | base: %04X",
        r[2], (r[2] & 0x0E) << 10);

    ImGui::Text("R5:  %02X  SAT base     | addr: %04X",
        r[5], (r[5] & 0x7E) << 7);

    ImGui::Text("R6:  %02X  spr pattern  | base: %04X",
        r[6], (r[6] & 0x04) << 11);

    ImGui::Text("R7:  %02X  border color | idx: %d",
        r[7], r[7] & 0x0F);

    ImGui::Text("R8:  %02X  H-scroll     | %d px",
        r[8], r[8]);

    ImGui::Text("R9:  %02X  V-scroll     | %d px",
        r[9], r[9]);

    ImGui::Text("R10: %02X  line counter | %d",
        r[10], r[10]);

    ImGui::Text("Line: %d", vdp.getCurrentLine());
}

// ─────────────────────────────────────────────────────────────────────────────
// drawPalette() — 32 color swatches from CRAM
// ─────────────────────────────────────────────────────────────────────────────

void VdpPanel::drawPalette()
{
    ImGui::Text("CRAM Palette (32 colors):");

    const uint8_t* cr = vdp.getCRAM();

    for (int i = 0; i < 32; i++) {
        // SMS: one byte per CRAM entry (BGR222)
        const uint8_t lo = cr[i];
        const float r = static_cast<float>((lo >> 4) & 0x03) / 3.0f;
        const float g = static_cast<float>((lo >> 2) & 0x03) / 3.0f;
        const float b = static_cast<float>( lo        & 0x03) / 3.0f;

        ImGui::PushID(i);
        ImGui::ColorButton("##c", ImVec4(r, g, b, 1.0f),
                           ImGuiColorEditFlags_None, ImVec2(18.0f, 18.0f));
        ImGui::PopID();

        // New row after every 16 swatches (palette 0 / palette 1 boundary)
        if ((i % 16) != 15) ImGui::SameLine();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// drawSprites() — SAT dump: index, X, Y, tile per sprite
// ─────────────────────────────────────────────────────────────────────────────

void VdpPanel::drawSprites()
{
    ImGui::Text("Sprite Attribute Table:");

    const uint8_t* vram = vdp.getVRAM();
    const uint8_t* r    = vdp.getRegs();

    const uint16_t satBase =
        static_cast<uint16_t>((r[5] & 0x7Eu) << 7);

    ImGui::Columns(4, "sprites");
    ImGui::Text("Idx");  ImGui::NextColumn();
    ImGui::Text("X");    ImGui::NextColumn();
    ImGui::Text("Y");    ImGui::NextColumn();
    ImGui::Text("Tile"); ImGui::NextColumn();
    ImGui::Separator();

    for (int i = 0; i < 64; i++) {
        const uint8_t y = vram[satBase + i];
        if (y == 0xD0) break;   // end-of-list sentinel

        const uint8_t x    = vram[satBase + 0x80 + i * 2];
        const uint8_t tile = vram[satBase + 0x80 + i * 2 + 1];

        ImGui::Text("%d", i);       ImGui::NextColumn();
        ImGui::Text("%d", x);       ImGui::NextColumn();
        ImGui::Text("%d", y + 1);   ImGui::NextColumn();
        ImGui::Text("%d", tile);    ImGui::NextColumn();
    }

    ImGui::Columns(1);
}
