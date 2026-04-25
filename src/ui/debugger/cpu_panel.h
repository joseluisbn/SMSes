// src/ui/debugger/cpu_panel.h
#pragma once

class Z80;

class CpuPanel {
public:
    explicit CpuPanel(Z80& cpu);

    void draw();
    bool isVisible()   const;
    void setVisible(bool v);

    bool isStepMode()  const;   // true when paused in step mode
    bool popStep();             // returns doStep then clears it

private:
    Z80&  cpu;
    bool  visible  = false;

    bool  stepMode = false;
    bool  doStep   = false;

    void drawRegisters();
    void drawFlags();
    void drawControls();
};
