// src/ui/app.h
#pragma once

#include "ui/menubar.h"
#include "ui/screen.h"
#include <SDL3/SDL.h>

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

    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool          running  = false;
    Screen        screen;
    Menubar       menubar;
};
