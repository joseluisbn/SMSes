// src/ui/debugger/debugger.cpp
#include "ui/debugger/debugger.h"
#include "system/sms.h"
#include "ui/menubar.h"

Debugger::Debugger(SMS& sms, Menubar& menubar)
    : sms(sms),
      menubar(menubar),
      cpuPanel(sms.getCPU()),
      memoryPanel(sms.getBus(), sms.getVDP()),
      vdpPanel(sms.getVDP()),
      disasmPanel(sms.getCPU(), sms.getBus()),
      psgPanel(sms.getPSG())
{}

void Debugger::draw()
{
    // Sync panel visibility from menubar toggles
    cpuPanel.setVisible(menubar.getCpuPanel());
    memoryPanel.setVisible(menubar.getMemoryPanel());
    vdpPanel.setVisible(menubar.getVdpPanel());
    disasmPanel.setVisible(menubar.getDisasmPanel());
    psgPanel.setVisible(menubar.getPsgPanel());

    // Draw all panels — each checks its own visible flag internally
    cpuPanel.draw();
    memoryPanel.draw();
    vdpPanel.draw();
    disasmPanel.draw();
    psgPanel.draw();
}

bool Debugger::shouldBreak()  const { return disasmPanel.shouldBreak(); }
bool Debugger::isStepMode()   const { return cpuPanel.isStepMode(); }
bool Debugger::popStep()            { return cpuPanel.popStep(); }
