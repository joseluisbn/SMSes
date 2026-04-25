// src/ui/debugger/memory_panel.h
#pragma once

#include <cstdint>

class Bus;
class VDP;

class MemoryPanel {
public:
    explicit MemoryPanel(Bus& bus, VDP& vdp);

    void draw();
    bool isVisible() const;
    void setVisible(bool v);

private:
    Bus&     bus;
    VDP&     vdp;
    bool     visible    = false;
    uint16_t baseAddr   = 0x0000;
    int      viewSource = 0;        // 0 = RAM, 1 = VRAM, 2 = ROM (paged)
    char     gotoAddrBuf[5] = "0000";

    void drawHexView(const uint8_t* data, int size,
                     uint16_t base, int bytesPerRow = 16);
};
