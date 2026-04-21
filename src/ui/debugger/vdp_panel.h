// src/ui/debugger/vdp_panel.h
#pragma once

#include "vdp/vdp.h"

class VdpPanel {
public:
    explicit VdpPanel(VDP& vdp);

    void draw();            // renders all visible sub-panels

    bool isVisible() const;
    void setVisible(bool v);

private:
    VDP&  vdp;
    bool  visible = false;

    // Sub-panel visibility toggles
    bool showRegisters = true;
    bool showPalette   = true;
    bool showTilemap   = false;
    bool showSprites   = false;

    void drawRegisters();   // R0–R10 decoded with field names
    void drawPalette();     // 32 color swatches from CRAM
    void drawTilemap();     // 512 tiles rendered as 8×8 swatches (future)
    void drawSprites();     // SAT dump: X, Y, tile index per sprite
};
