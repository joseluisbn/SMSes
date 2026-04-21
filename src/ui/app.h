// src/ui/app.h
#pragma once

#include "ui/menubar.h"
#include "ui/screen.h"
#include "vdp/vdp.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <vector>

class App {
public:
    bool init(const char* title, int width, int height);
    void run();
    void shutdown();

private:
    void processEvents();
    void update();
    void render();
    void handleMenuActions();

    void loadROMFromFile(const std::string& path);
    std::string buildTitle() const;

    SDL_Window*            window        = nullptr;
    SDL_Renderer*          renderer      = nullptr;
    bool                   running       = false;
    Screen                 screen;
    Menubar                menubar;
    std::vector<uint8_t>   romData;
    bool                   romLoaded     = false;
    Region                 currentRegion = Region::NTSC;
    std::string            romFilename;  // set on load, used by buildTitle()
};
