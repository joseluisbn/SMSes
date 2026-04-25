// src/ui/app.cpp
#include "ui/app.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <array>
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

    if (!initAudio())
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
    static std::array<float, AUDIO_BUFFER_SAMPLES * 2 * PSG::AUDIO_CHANNELS> audioBuffer;
    const int samplesNeeded = static_cast<int>(
        PSG::SAMPLE_RATE / (currentRegion == Region::NTSC ? 59.92 : 49.70));

    psg.generateSamples(audioBuffer.data(), samplesNeeded);

    if (audioStream)
        SDL_PutAudioStreamData(audioStream, audioBuffer.data(),
            samplesNeeded * PSG::AUDIO_CHANNELS * static_cast<int>(sizeof(float)));
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
    if (menubar.popRegionNTSC()) {
        currentRegion = Region::NTSC;
        psg.setClockHz(3579545.0);
        SDL_SetWindowTitle(window, buildTitle().c_str());
    }
    if (menubar.popRegionPAL()) {
        currentRegion = Region::PAL;
        psg.setClockHz(3546895.0);
        SDL_SetWindowTitle(window, buildTitle().c_str());
    }
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
    romLoaded   = true;
    romFilename = std::filesystem::path(path).filename().string();
    std::fprintf(stderr, "ROM loaded: %s (%lld bytes)\n", path.c_str(),
                 static_cast<long long>(size));

    // Auto-detect region from ROM header.
    // Bus is not yet available in Phase 4, so we scan romData directly
    // using the same logic as Mapper::detectRegion().
    auto detectRegion = [&]() -> Region {
        if (romData.size() < 0x8000) return Region::NTSC;
        const char magic[8] = {'T','M','R',' ','S','E','G','A'};
        for (int i = 0; i < 8; i++)
            if (romData[0x7FF0 + i] != static_cast<uint8_t>(magic[i]))
                return Region::NTSC;
        const uint8_t code = (romData[0x7FFF] >> 4) & 0x0Fu;
        // codes 3 (SMS Japan) and 5 (GG Japan) → NTSC; all others → PAL
        return (code == 3 || code == 5) ? Region::NTSC : Region::PAL;
    };

    currentRegion = detectRegion();
    psg.setClockHz(currentRegion == Region::PAL ? 3546895.0 : 3579545.0);

    const char* regionStr = (currentRegion == Region::PAL) ? "PAL" : "NTSC";
    SDL_SetWindowTitle(window, buildTitle().c_str());
    std::fprintf(stderr, "Detected region: %s\n", regionStr);
}

std::string App::buildTitle() const {
    const char* regionTag = (currentRegion == Region::PAL) ? " [PAL]" : " [NTSC]";
    if (romLoaded && !romFilename.empty())
        return "SMS Emulator — " + romFilename + regionTag;
    return "SMS Emulator";
}

void App::run() {
    while (running) {
        processEvents();
        update();
        render();
    }
}

bool App::initAudio()
{
    SDL_AudioSpec spec{};
    spec.format   = SDL_AUDIO_F32;
    spec.channels = PSG::AUDIO_CHANNELS;
    spec.freq     = PSG::SAMPLE_RATE;

    audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (audioDevice == 0) {
        std::fprintf(stderr, "SDL Audio error: %s\n", SDL_GetError());
        return false;
    }

    audioStream = SDL_CreateAudioStream(&spec, &spec);
    if (!audioStream) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
        std::fprintf(stderr, "SDL AudioStream error: %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_BindAudioStream(audioDevice, audioStream)) {
        SDL_DestroyAudioStream(audioStream);
        audioStream = nullptr;
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
        std::fprintf(stderr, "SDL BindAudioStream error: %s\n", SDL_GetError());
        return false;
    }

    SDL_ResumeAudioDevice(audioDevice);
    psg.reset();
    return true;
}

void App::shutdownAudio()
{
    if (audioDevice) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
    if (audioStream) {
        SDL_DestroyAudioStream(audioStream);
        audioStream = nullptr;
    }
}

void App::shutdown() {
    shutdownAudio();
    screen.shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
