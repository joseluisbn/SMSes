// src/ui/debugger/psg_panel.cpp
#include "ui/debugger/psg_panel.h"
#include "psg/psg.h"
#include <imgui.h>
#include <string>

PsgPanel::PsgPanel(PSG& psg)
    : psg(psg)
{
}

bool PsgPanel::isVisible() const
{
    return visible;
}

void PsgPanel::setVisible(bool v)
{
    visible = v;
}

void PsgPanel::drawChannelRow(const char* label, float volume,
                               uint16_t period, double clockHz,
                               const float* history)
{
    const double freq = (period > 0) ? (clockHz / (2.0 * period)) : 0.0;
    ImGui::Text("%-8s %6.3f %8d %7.1fHz", label, volume, period, freq);
    ImGui::SameLine();
    ImGui::PlotLines(("##" + std::string(label)).c_str(),
                     history, HISTORY_SIZE, historyIdx,
                     nullptr, -1.0f, 1.0f, ImVec2(120, 30));
}

void PsgPanel::drawMixer()
{
    float mixed = 0.0f;
    for (int i = 0; i < 3; ++i)
        mixed += PSG::ATTENUATION_TABLE[psg.getTone(i).volume];
    mixed += PSG::ATTENUATION_TABLE[psg.getNoise().volume];
    mixed /= 4.0f;

    ImGui::Text("Master output:");
    ImGui::SameLine();
    ImGui::ProgressBar(mixed, ImVec2(200, 0));
}

void PsgPanel::draw()
{
    if (!visible) return;

    ImGui::SetNextWindowSize(ImVec2(420, 300), ImGuiCond_Always);
    ImGui::Begin("PSG Monitor", &visible, ImGuiWindowFlags_NoResize);

    const auto& t0 = psg.getTone(0);
    const auto& t1 = psg.getTone(1);
    const auto& t2 = psg.getTone(2);
    const auto& n  = psg.getNoise();

    ch0History[historyIdx]   = PSG::ATTENUATION_TABLE[t0.volume] * static_cast<float>(t0.output);
    ch1History[historyIdx]   = PSG::ATTENUATION_TABLE[t1.volume] * static_cast<float>(t1.output);
    ch2History[historyIdx]   = PSG::ATTENUATION_TABLE[t2.volume] * static_cast<float>(t2.output);
    noiseHistory[historyIdx] = PSG::ATTENUATION_TABLE[n.volume]  * static_cast<float>(n.output);
    historyIdx = (historyIdx + 1) % HISTORY_SIZE;

    constexpr double clockHz = 3579545.0 / 16.0;

    ImGui::Text("SN76489 PSG Monitor");
    ImGui::Separator();

    ImGui::Text("%-8s %6s %8s %8s %s", "Channel", "Vol", "Period", "Freq", "Waveform");
    ImGui::Separator();

    drawChannelRow("Tone 0", PSG::ATTENUATION_TABLE[t0.volume], t0.period, clockHz, ch0History.data());
    drawChannelRow("Tone 1", PSG::ATTENUATION_TABLE[t1.volume], t1.period, clockHz, ch1History.data());
    drawChannelRow("Tone 2", PSG::ATTENUATION_TABLE[t2.volume], t2.period, clockHz, ch2History.data());

    const char* noiseType = (n.control & 0x04) ? "White" : "Periodic";
    ImGui::Text("%-8s %6.3f %8s %8s",
                "Noise", PSG::ATTENUATION_TABLE[n.volume],
                noiseType, "\xe2\x80\x94");
    ImGui::SameLine();
    ImGui::PlotLines("##noise", noiseHistory.data(), HISTORY_SIZE,
                     historyIdx, nullptr, -1.0f, 1.0f, ImVec2(120, 30));

    ImGui::Separator();
    drawMixer();

    ImGui::End();
}
