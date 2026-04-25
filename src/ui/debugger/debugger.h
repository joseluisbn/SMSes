// src/ui/debugger/debugger.h
#pragma once

#include "ui/debugger/cpu_panel.h"
#include "ui/debugger/disasm_panel.h"
#include "ui/debugger/memory_panel.h"
#include "ui/debugger/psg_panel.h"
#include "ui/debugger/vdp_panel.h"

class SMS;
class Menubar;

class Debugger {
public:
    Debugger(SMS& sms, Menubar& menubar);

    void draw();

    // Breakpoint / step-mode queries — called by App::update()
    bool shouldBreak()  const;  // true if any enabled BP matches current PC
    bool isStepMode()   const;  // true when CPU panel is in step mode
    bool popStep();             // consume the pending single-step request

private:
    SMS&     sms;
    Menubar& menubar;

    CpuPanel    cpuPanel;
    MemoryPanel memoryPanel;  // takes Bus& and VDP&
    VdpPanel    vdpPanel;
    DisasmPanel disasmPanel;
    PsgPanel    psgPanel;
};
