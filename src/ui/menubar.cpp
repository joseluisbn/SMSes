// src/ui/menubar.cpp
#include "ui/menubar.h"
#include <algorithm>
#include <imgui.h>

void Menubar::draw() {
    if (!ImGui::BeginMainMenuBar())
        return;

    // ── File ────────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open ROM..."))
            actionOpenRom = true;
        ImGui::Separator();
        ImGui::Text("Save Slot:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputInt("##slot", &selectedSlot, 1, 1);
        selectedSlot = std::clamp(selectedSlot, 0, 9);
        if (ImGui::MenuItem("Save State  (F5)"))
            actionSaveState = true;
        if (ImGui::MenuItem("Load State  (F8)"))
            actionLoadState = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Exit"))
            actionExit = true;
        ImGui::EndMenu();
    }

    // ── Emulation ───────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Emulation")) {
        ImGui::BeginDisabled(!isPaused);
        if (ImGui::MenuItem("Play"))
            isPaused = false;
        ImGui::EndDisabled();

        ImGui::BeginDisabled(isPaused);
        if (ImGui::MenuItem("Pause"))
            isPaused = true;
        ImGui::EndDisabled();

        if (ImGui::MenuItem("Reset"))
            actionReset = true;

        ImGui::Separator();
        ImGui::Text("Region:");
        if (ImGui::MenuItem("NTSC (60Hz)", nullptr, selectedRegion == Region::NTSC)) {
            selectedRegion   = Region::NTSC;
            actionRegionNTSC = true;
        }
        if (ImGui::MenuItem("PAL  (50Hz)", nullptr, selectedRegion == Region::PAL)) {
            selectedRegion  = Region::PAL;
            actionRegionPAL = true;
        }
        ImGui::EndMenu();
    }

    // ── Debug ───────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Debug")) {
        ImGui::MenuItem("CPU Registers",  nullptr, &showCpuPanel);
        ImGui::MenuItem("Disassembler",   nullptr, &showDisasmPanel);
        ImGui::MenuItem("Memory Viewer",  nullptr, &showMemoryPanel);
        ImGui::MenuItem("VDP Viewer",     nullptr, &showVdpPanel);
        ImGui::MenuItem("PSG Monitor",    nullptr, &showPsgPanel);
        ImGui::EndMenu();
    }

    // ── View ────────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Scale 1x"))
            actionScale1x = true;
        if (ImGui::MenuItem("Scale 2x"))
            actionScale2x = true;
        if (ImGui::MenuItem("Scale 3x"))
            actionScale3x = true;        ImGui::Separator();
        if (ImGui::MenuItem("Turbo (uncapped)", nullptr, turboEnabled))
            actionTurbo = true;        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

// ── Panel visibility getters ────────────────────────────────────────────────────
bool Menubar::getCpuPanel()    const { return showCpuPanel;    }
bool Menubar::getDisasmPanel() const { return showDisasmPanel; }
bool Menubar::getMemoryPanel() const { return showMemoryPanel; }
bool Menubar::getVdpPanel()    const { return showVdpPanel;    }
bool Menubar::getPsgPanel()    const { return showPsgPanel;    }
bool Menubar::getIsPaused()    const { return isPaused;        }

// ── One-shot action pops ─────────────────────────────────────────────────────────
bool Menubar::popOpenRom() { bool v = actionOpenRom; actionOpenRom = false; return v; }
bool Menubar::popReset()   { bool v = actionReset;   actionReset   = false; return v; }
bool Menubar::popExit()    { bool v = actionExit;    actionExit    = false; return v; }
bool Menubar::popScale1x() { bool v = actionScale1x; actionScale1x = false; return v; }
bool Menubar::popScale2x() { bool v = actionScale2x; actionScale2x = false; return v; }
bool Menubar::popScale3x() { bool v = actionScale3x; actionScale3x = false; return v; }
bool Menubar::popRegionNTSC() { bool v = actionRegionNTSC; actionRegionNTSC = false; return v; }
bool Menubar::popRegionPAL()  { bool v = actionRegionPAL;  actionRegionPAL  = false; return v; }
bool Menubar::popTurbo()      { bool v = actionTurbo; actionTurbo = false;
                                 if (v) turboEnabled = !turboEnabled; return v; }
bool Menubar::popSaveState()  { bool v = actionSaveState; actionSaveState = false; return v; }
bool Menubar::popLoadState()  { bool v = actionLoadState; actionLoadState = false; return v; }
int  Menubar::getSelectedSlot() const { return selectedSlot; }

Region Menubar::getSelectedRegion() const { return selectedRegion; }
