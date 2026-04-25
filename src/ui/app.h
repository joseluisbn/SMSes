// src/ui/app.h
#pragma once

#include "system/sms.h"
#include "ui/debugger/debugger.h"
#include "ui/menubar.h"
#include "ui/screen.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <vector>

class App {
public:
    App();  // required: initializes debugger(sms, menubar)
    bool init(const char* title, int width, int height);
    void run();
    void shutdown();

    Region getCurrentRegion() const { return sms.getRegion(); }

private:
    void processEvents();
    void update();
    void render();
    void handleMenuActions();

    void loadROMFromFile(const std::string& path);
    std::string buildTitle() const;

    bool initAudio();
    void shutdownAudio();

    SDL_Window*            window       = nullptr;
    SDL_Renderer*          renderer     = nullptr;
    bool                   running      = false;

    // Declaration order matters: sms and menubar must be constructed
    // before debugger (which holds references to both).
    SMS                    sms;
    Menubar                menubar;
    Screen                 screen;
    Debugger               debugger;   // initialized via App() MIL

    std::string            romFilename;  // set on load, used by buildTitle()

    uint8_t                joypad1State = 0;
    uint8_t                joypad2State = 0;

    // Frame timing
    uint64_t               lastFrameTime = 0;    // SDL_GetTicks64() at last frame
    double                 frameAccum    = 0.0;  // accumulated time debt in ms
    bool                   uncapped      = false; // turbo mode

    // Save states
    int                    currentSaveSlot = 0;

    static constexpr int   AUDIO_BUFFER_SAMPLES = 512;
    SDL_AudioDeviceID      audioDevice  = 0;
    SDL_AudioStream*       audioStream  = nullptr;
};
