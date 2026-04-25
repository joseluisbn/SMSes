// src/ui/debugger/memory_panel.cpp
#include "ui/debugger/memory_panel.h"
#include "memory/bus.h"
#include "vdp/vdp.h"
#include <imgui.h>
#include <array>
#include <cctype>
#include <cstdlib>

MemoryPanel::MemoryPanel(Bus& bus, VDP& vdp)
    : bus(bus), vdp(vdp)
{}

bool MemoryPanel::isVisible() const
{
    return visible;
}

void MemoryPanel::setVisible(bool v)
{
    visible = v;
}

void MemoryPanel::drawHexView(const uint8_t* data, int size,
                               uint16_t base, int bytesPerRow)
{
    ImGui::BeginChild("##hexview", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    const int rows = (size + bytesPerRow - 1) / bytesPerRow;

    for (int row = 0; row < rows; ++row) {
        const int rowStart = row * bytesPerRow;

        // Address
        ImGui::Text("%04X: ", static_cast<unsigned>(base + rowStart));
        ImGui::SameLine();

        // Hex bytes
        for (int col = 0; col < bytesPerRow; ++col) {
            const int idx = rowStart + col;
            if (idx < size)
                ImGui::Text("%02X ", data[idx]);
            else
                ImGui::Text("   ");
            ImGui::SameLine();
        }

        // ASCII
        ImGui::Text("  ");
        ImGui::SameLine();

        for (int col = 0; col < bytesPerRow; ++col) {
            const int idx = rowStart + col;
            if (idx < size) {
                const char c = static_cast<char>(data[idx]);
                ImGui::Text("%c", std::isprint(static_cast<unsigned char>(c)) ? c : '.');
            } else {
                ImGui::Text(" ");
            }
            if (col < bytesPerRow - 1) ImGui::SameLine();
        }
    }

    ImGui::EndChild();
}

void MemoryPanel::draw()
{
    if (!visible) return;

    ImGui::Begin("Memory Viewer", &visible);

    // Source selector
    ImGui::RadioButton("RAM",  &viewSource, 0); ImGui::SameLine();
    ImGui::RadioButton("VRAM", &viewSource, 1); ImGui::SameLine();
    ImGui::RadioButton("ROM",  &viewSource, 2);

    // Go-to address
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    if (ImGui::InputText("Goto", gotoAddrBuf, sizeof(gotoAddrBuf),
                         ImGuiInputTextFlags_CharsHexadecimal |
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        baseAddr = static_cast<uint16_t>(std::strtoul(gotoAddrBuf, nullptr, 16));
    }

    ImGui::Separator();

    switch (viewSource) {
        case 0:
            // 8 KiB system RAM, mapped at 0xC000
            drawHexView(bus.getMapper().getRamPtr(),
                        static_cast<int>(Mapper::RAM_SIZE), 0xC000);
            break;

        case 1:
            // 16 KiB VRAM
            drawHexView(vdp.getVRAM(), 16384, 0x0000);
            break;

        case 2: {
            // 256-byte ROM snapshot via bus reads from baseAddr
            static std::array<uint8_t, 256> romSnap;
            for (int i = 0; i < 256; ++i)
                romSnap[i] = bus.read(static_cast<uint16_t>(baseAddr + i));
            drawHexView(romSnap.data(), 256, baseAddr);
            break;
        }

        default:
            break;
    }

    ImGui::End();
}
