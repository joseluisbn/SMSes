// src/ui/app.cpp
#include "ui/app.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <cstdio>
#include <filesystem>
#include <fstream>

bool App::init(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS))
        return false;

    window = SDL_CreateWindow(title, width, height,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window)
        return false;

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
        return false;

    SDL_SetRenderVSync(renderer, 1);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    if (!screen.init(renderer))
        return false;

    running = true;
    return true;
}

void App::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE)
                    running = false;
                break;
            case SDL_EVENT_DROP_FILE:
                if (event.drop.data)
                    loadROMFromFile(event.drop.data);
                break;
            default:
                break;
        }
    }
}

void App::update() {
    // Empty for Phase 1. Will call sms.step() in future phases.
}

void App::handleMenuActions() {
    if (menubar.popExit())
        running = false;
    if (menubar.popScale1x())
        screen.setScale(1.0f);
    if (menubar.popScale2x())
        screen.setScale(2.0f);
    if (menubar.popScale3x())
        screen.setScale(3.0f);
    if (menubar.popOpenRom())
        std::fprintf(stderr, "Open ROM dialog: not implemented — drop a .sms file onto the window\n");
    if (menubar.popReset())
        std::fprintf(stderr, "Reset: not implemented yet\n");
}

void App::render() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    menubar.draw();
    handleMenuActions();
    screen.draw();

    ImGui::Render();
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void App::loadROMFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::fprintf(stderr, "ROM load failed: cannot open '%s'\n", path.c_str());
        return;
    }
    const auto size = file.tellg();
    file.seekg(0);
    romData.resize(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(romData.data()), size);
    romLoaded = true;
    std::fprintf(stderr, "ROM loaded: %s (%lld bytes)\n", path.c_str(),
                 static_cast<long long>(size));
    // Update window title with the ROM filename
    const std::string filename =
        std::filesystem::path(path).filename().string();
    SDL_SetWindowTitle(window, ("SMS Emulator \u2014 " + filename).c_str());
}

void App::run() {
    while (running) {
        processEvents();
        update();
        render();
    }
}

void App::shutdown() {
    screen.shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
