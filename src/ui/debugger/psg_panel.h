// src/ui/debugger/psg_panel.h
#pragma once

#include <array>
#include <cstdint>

class PSG;

class PsgPanel {
public:
    explicit PsgPanel(PSG& psg);

    void draw();
    bool isVisible() const;
    void setVisible(bool v);

private:
    PSG&  psg;
    bool  visible = false;

    static constexpr int HISTORY_SIZE = 256;
    std::array<float, HISTORY_SIZE> ch0History   {};
    std::array<float, HISTORY_SIZE> ch1History   {};
    std::array<float, HISTORY_SIZE> ch2History   {};
    std::array<float, HISTORY_SIZE> noiseHistory {};
    int historyIdx = 0;

    void drawChannelRow(const char* label, float volume,
                        uint16_t period, double clockHz,
                        const float* history);
    void drawMixer();
};
