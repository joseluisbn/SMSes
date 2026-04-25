// src/psg/psg.h
#pragma once

#include <cstdint>

// Forward-declare save-state struct to avoid circular include with save_state.h
struct PSGSaveState;

struct ToneChannel {
    uint16_t period  = 0;
    uint16_t counter = 0;
    int      output  = 1;
    uint8_t  volume  = 0x0F;
};

struct NoiseChannel {
    uint8_t  control  = 0;
    uint16_t period   = 0;
    uint16_t counter  = 0;
    uint16_t shiftReg = 0x8000;
    int      output   = 1;
    uint8_t  volume   = 0x0F;
};

class PSG {
public:
    static constexpr int   SAMPLE_RATE    = 44100;
    static constexpr int   AUDIO_CHANNELS = 2;
    static constexpr float VOLUME_MAX     = 0.25f;

    static constexpr float ATTENUATION_TABLE[16] = {
        1.000f, 0.794f, 0.631f, 0.501f,
        0.398f, 0.316f, 0.251f, 0.200f,
        0.158f, 0.126f, 0.100f, 0.079f,
        0.063f, 0.050f, 0.040f, 0.000f
    };

    PSG();

    void reset();
    void setClockHz(double hz);
    void write(uint8_t val);

    void generateSamples(float* buffer, int numSamples);

    const ToneChannel&  getTone(int ch) const;
    const NoiseChannel& getNoise()      const;

    // Save state
    PSGSaveState captureState() const;
    void         loadState(const PSGSaveState& s);

private:
    double clockHz      = 3579545.0 / 16.0;
    double clockPeriod  = 1.0 / (3579545.0 / 16.0);
    double samplePeriod = 1.0 / SAMPLE_RATE;
    double accumulator  = 0.0;

    ToneChannel  tone[3];
    NoiseChannel noise;

    uint8_t latchedChannel  = 0;
    bool    latchedIsVolume = false;

    void  setPeriodLow(int ch, uint8_t data);
    void  setPeriodHigh(int ch, uint8_t data);
    void  setVolume(int ch, uint8_t data);
    void  clockTone(ToneChannel& ch);
    void  clockNoise();
    float mixSample();
    void  updateNoisePeriod();
};
