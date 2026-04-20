// src/main.cpp
#include "ui/app.h"
#include <SDL3/SDL_main.h>

int main(int argc, char* argv[]) {
    App app;
    if (!app.init("SMS Emulator", 840, 420))
        return 1;
    app.run();
    app.shutdown();
    return 0;
}
