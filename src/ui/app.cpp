// src/ui/app.cpp
#include "ui/app.h"
#include "io/io.h"
#include "psg/psg.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>

App::App()
    : debugger(sms, menubar)
{}

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
    auto updateJoy = [&](SDL_Keycode key, bool pressed) {
        uint8_t btn = 0;
        switch (key) {
            case SDLK_UP:    btn = JOY_UP;    break;
            case SDLK_DOWN:  btn = JOY_DOWN;  break;
            case SDLK_LEFT:  btn = JOY_LEFT;  break;
            case SDLK_RIGHT: btn = JOY_RIGHT; break;
            case SDLK_Z:     btn = JOY_FIRE1; break;
            case SDLK_X:     btn = JOY_FIRE2; break;
            default: break;
        }
        if (pressed)  joypad1State |= btn;
        else          joypad1State &= static_cast<uint8_t>(~btn);
        sms.setJoypad1(joypad1State);
    };

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
                else
                    updateJoy(event.key.key, true);
                break;
            case SDL_EVENT_KEY_UP:
                updateJoy(event.key.key, false);
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
    if (!sms.isROMLoaded()) return;

    // Breakpoint check — halt before executing if PC hits an enabled BP
    if (debugger.shouldBreak()) return;

    // Step mode — execute exactly one instruction per button press
    if (debugger.isStepMode()) {
        if (debugger.popStep()) {
            const int cycles = sms.getCPU().step();
            sms.getVDP().tick(cycles);
            if (sms.getVDP().getIRQ()) {
                sms.getVDP().clearIRQ();
                sms.getCPU().irq();
            }
            if (sms.getVDP().pollFrameReady())
                screen.update(sms.getVDP().getFramebuffer());
        }
        return;
    }

    // Normal emulation: run one full frame
    sms.runFrame();

    // Push completed framebuffer to screen texture
    if (sms.getVDP().pollFrameReady())
        screen.update(sms.getVDP().getFramebuffer());

    // Generate and queue audio for this frame
    const int samplesNeeded = static_cast<int>(
        PSG::SAMPLE_RATE / (sms.getRegion() == Region::NTSC ? 59.92 : 49.70));

    static std::array<float, 1024 * PSG::AUDIO_CHANNELS> audioBuffer;
    sms.fillAudio(audioBuffer.data(), samplesNeeded);

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
    if (menubar.popReset()) {
        sms.reset();
        SDL_SetWindowTitle(window, buildTitle().c_str());
    }
    if (menubar.popRegionNTSC()) {
        sms.setRegion(Region::NTSC);
        SDL_SetWindowTitle(window, buildTitle().c_str());
    }
    if (menubar.popRegionPAL()) {
        sms.setRegion(Region::PAL);
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
    debugger.draw();

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
    std::vector<uint8_t> romData(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(romData.data()), size);

    std::fprintf(stderr, "ROM loaded: %s (%lld bytes)\n", path.c_str(),
                 static_cast<long long>(size));

    if (!sms.loadROM(romData)) {
        std::fprintf(stderr, "Failed to load ROM: invalid format\n");
        return;
    }
    sms.reset();

    romFilename = std::filesystem::path(path).filename().string();
    const char* regionStr = (sms.getRegion() == Region::PAL) ? "PAL" : "NTSC";
    SDL_SetWindowTitle(window, buildTitle().c_str());
    std::fprintf(stderr, "Detected region: %s\n", regionStr);
}

std::string App::buildTitle() const {
    const char* regionTag = (sms.getRegion() == Region::PAL) ? " [PAL]" : " [NTSC]";
    if (sms.isROMLoaded() && !romFilename.empty())
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
