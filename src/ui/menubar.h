// src/ui/menubar.h
#pragma once

#include "vdp/vdp.h"

class Menubar {
public:
    void draw();

    // Getters for panel visibility (const)
    bool getCpuPanel()    const;
    bool getDisasmPanel() const;
    bool getMemoryPanel() const;
    bool getVdpPanel()    const;
    bool getPsgPanel()    const;

    // Getters for one-shot actions — return value then reset to false
    bool popOpenRom();
    bool popReset();
    bool popExit();
    bool popScale1x();
    bool popScale2x();
    bool popScale3x();
    bool popRegionNTSC();
    bool popRegionPAL();
    bool popTurbo();             // toggle turbo (uncapped) mode
    bool popSaveState();
    bool popLoadState();
    int  getSelectedSlot() const;

    bool         getIsPaused()        const;
    Region       getSelectedRegion()  const;

private:
    // Debug panels visibility toggles
    bool showCpuPanel    = false;
    bool showDisasmPanel = false;
    bool showMemoryPanel = false;
    bool showVdpPanel    = false;
    bool showPsgPanel    = false;

    // Emulation state
    bool isPaused = false;

    // Pending actions (set true for one frame, App reads and clears them)
    bool actionOpenRom    = false;
    bool actionReset      = false;
    bool actionExit       = false;
    bool actionScale1x    = false;
    bool actionScale2x    = false;
    bool actionScale3x    = false;
    bool actionRegionNTSC = false;
    bool actionRegionPAL  = false;
    bool actionTurbo      = false;
    bool turboEnabled     = false;
    bool actionSaveState  = false;
    bool actionLoadState  = false;

    // Save state slot
    int selectedSlot = 0;

    // Region selection
    Region selectedRegion = Region::NTSC;
};
