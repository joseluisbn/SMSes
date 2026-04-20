// src/ui/screen.h
#pragma once

#include <SDL3/SDL.h>

class Screen {
public:
    static constexpr int SMS_WIDTH  = 256;
    static constexpr int SMS_HEIGHT = 192;

    // Creates the streaming RGBA texture. Returns false on failure.
    bool init(SDL_Renderer* renderer);

    // Uploads a new framebuffer (RGBA, pitch = SMS_WIDTH * 4).
    void update(const uint32_t* framebuffer);

    // Renders the screen as an ImGui window.
    void draw();

    void setScale(float s);
    void shutdown();

private:
    SDL_Renderer* renderer  = nullptr;  // non-owning
    SDL_Texture*  texture   = nullptr;  // owning
    float         scale     = 2.0f;
    bool          hasUpdate = false;
};
