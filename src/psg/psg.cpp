// src/psg/psg.cpp
#include "psg/psg.h"
#include "system/save_state.h"

PSG::PSG()
{
    reset();
}

void PSG::reset()
{
    for (int i = 0; i < 3; ++i) {
        tone[i].period  = 0;
        tone[i].counter = 0;
        tone[i].output  = 1;
        tone[i].volume  = 0x0F;
    }

    noise.control  = 0;
    noise.period   = 0;
    noise.shiftReg = 0x8000;
    noise.counter  = 0;
    noise.output   = 1;
    noise.volume   = 0x0F;

    latchedChannel  = 0;
    latchedIsVolume = false;
    accumulator     = 0.0;
}

void PSG::setClockHz(double hz)
{
    clockHz      = hz / 16.0;
    clockPeriod  = 1.0 / clockHz;
    samplePeriod = 1.0 / SAMPLE_RATE;
}

// ---- Private helpers -------------------------------------------------------

void PSG::setVolume(int ch, uint8_t data)
{
    if (ch < 3)
        tone[ch].volume = data & 0x0F;
    else
        noise.volume = data & 0x0F;
}

void PSG::setPeriodLow(int ch, uint8_t data)
{
    if (ch < 3) {
        tone[ch].period = (tone[ch].period & 0x3F0) | (data & 0x0F);
    } else {
        noise.control  = data & 0x07;
        noise.shiftReg = 0x8000;  // reset LFSR when noise control changes
        updateNoisePeriod();
    }
}

void PSG::setPeriodHigh(int ch, uint8_t data)
{
    if (ch < 3) {
        tone[ch].period = (tone[ch].period & 0x00F) | (static_cast<uint16_t>(data) << 4);
    }
    // Noise channel has no high byte — ignore
}

void PSG::updateNoisePeriod()
{
    // Rates relative to master clock (N): N/512, N/1024, N/2048.
    // PSG clock = N/16, so LFSR fires at PSG_clock/period.
    // N/512 = PSG_clock/32 → period 0x20; N/1024 → 0x40; N/2048 → 0x80.
    switch (noise.control & 0x03) {
        case 0: noise.period = 0x20;              break;   // N/512
        case 1: noise.period = 0x40;              break;   // N/1024
        case 2: noise.period = 0x80;              break;   // N/2048
        case 3: noise.period = tone[2].period;    break;   // tone 2
    }
}

void PSG::clockTone(ToneChannel& ch)
{
    if (ch.period == 0) return;

    --ch.counter;
    if (ch.counter <= 0) {
        ch.counter = ch.period;
        ch.output  = -ch.output;
    }
}

void PSG::clockNoise()
{
    --noise.counter;
    if (noise.counter <= 0) {
        noise.counter = (noise.period > 0) ? noise.period : 0x10;

        bool feedback;
        if (noise.control & 0x04) {
            // White noise: tapped bits 0 and 3 (SMS-specific)
            feedback = ((noise.shiftReg & 0x0001) ^ ((noise.shiftReg >> 3) & 0x0001)) != 0;
        } else {
            // Periodic noise
            feedback = (noise.shiftReg & 0x0001) != 0;
        }

        noise.shiftReg = (noise.shiftReg >> 1) | (feedback ? 0x8000u : 0u);
        noise.output   = (noise.shiftReg & 0x0001) ? 1 : -1;
    }
}

float PSG::mixSample()
{
    float sample = 0.0f;

    for (int i = 0; i < 3; ++i)
        sample += static_cast<float>(tone[i].output) * ATTENUATION_TABLE[tone[i].volume];

    sample += static_cast<float>(noise.output) * ATTENUATION_TABLE[noise.volume];

    return sample * VOLUME_MAX;
}

// ---- Public API ------------------------------------------------------------

void PSG::write(uint8_t val)
{
    if (val & 0x80) {
        // LATCH byte
        latchedChannel  = (val >> 5) & 0x03;
        latchedIsVolume = ((val >> 4) & 0x01) != 0;
        uint8_t data    = val & 0x0F;

        if (latchedIsVolume)
            setVolume(latchedChannel, data);
        else
            setPeriodLow(latchedChannel, data);
    } else {
        // DATA byte
        uint8_t data = val & 0x3F;

        if (latchedIsVolume)
            setVolume(latchedChannel, data & 0x0F);
        else
            setPeriodHigh(latchedChannel, data);
    }
}

void PSG::generateSamples(float* buffer, int numSamples)
{
    const double cyclesPerSample = clockHz / static_cast<double>(SAMPLE_RATE);

    for (int i = 0; i < numSamples; ++i) {
        accumulator += cyclesPerSample;

        while (accumulator >= 1.0) {
            accumulator -= 1.0;
            clockTone(tone[0]);
            clockTone(tone[1]);
            clockTone(tone[2]);
            clockNoise();

            if ((noise.control & 0x03) == 3)
                noise.period = tone[2].period;
        }

        const float sample  = mixSample();
        buffer[i * 2 + 0]   = sample;  // left
        buffer[i * 2 + 1]   = sample;  // right
    }
}

// ---- Debugger accessors ----------------------------------------------------

const ToneChannel& PSG::getTone(int ch) const
{
    return tone[ch];
}

const NoiseChannel& PSG::getNoise() const
{
    return noise;
}

PSGSaveState PSG::captureState() const
{
    PSGSaveState s;
    s.tone[0]         = tone[0];
    s.tone[1]         = tone[1];
    s.tone[2]         = tone[2];
    s.noise           = noise;
    s.latchedChannel  = latchedChannel;
    s.latchedIsVolume = latchedIsVolume;
    s.clockHz         = clockHz;
    return s;
}

void PSG::loadState(const PSGSaveState& s)
{
    tone[0]         = s.tone[0];
    tone[1]         = s.tone[1];
    tone[2]         = s.tone[2];
    noise           = s.noise;
    latchedChannel  = s.latchedChannel;
    latchedIsVolume = s.latchedIsVolume;
    clockHz         = s.clockHz;
    clockPeriod     = (clockHz > 0.0) ? 1.0 / clockHz : 0.0;
    accumulator     = 0.0;  // reset to avoid audio glitch on restore
}
