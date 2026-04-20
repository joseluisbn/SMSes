// src/ui/app.cpp
#include "ui/app.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <cstdio>

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
        if (event.type == SDL_EVENT_QUIT)
            running = false;
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
            running = false;
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
        std::fprintf(stderr, "Open ROM: not implemented yet\n");
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
