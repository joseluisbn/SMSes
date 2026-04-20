// src/ui/screen.cpp
#include "ui/screen.h"
#include <imgui.h>
#include <SDL3/SDL.h>

bool Screen::init(SDL_Renderer* r) {
    renderer  = r;
    texture   = SDL_CreateTexture(renderer,
                                  SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  SMS_WIDTH, SMS_HEIGHT);
    hasUpdate = false;
    return texture != nullptr;
}

void Screen::update(const uint32_t* framebuffer) {
    if (!texture)
        return;
    SDL_UpdateTexture(texture, nullptr, framebuffer, SMS_WIDTH * 4);
    hasUpdate = true;
}

void Screen::draw() {
    const ImVec2 winSize(SMS_WIDTH  * scale + 16.0f,
                         SMS_HEIGHT * scale + 36.0f);
    ImGui::SetNextWindowSize(winSize);
    ImGui::Begin("Master System", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    const ImVec2 imgSize(SMS_WIDTH * scale, SMS_HEIGHT * scale);

    if (!texture || !hasUpdate) {
        // Draw gray placeholder rectangle
        const ImVec2 p  = ImGui::GetCursorScreenPos();
        ImDrawList*  dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(p,
                          ImVec2(p.x + imgSize.x, p.y + imgSize.y),
                          IM_COL32(64, 64, 64, 255));

        // Overlay text only when texture exists but no frame has been pushed yet
        if (texture && !hasUpdate) {
            const char*  msg      = "Load a ROM to start";
            const ImVec2 textSize = ImGui::CalcTextSize(msg);
            const ImVec2 textPos(p.x + (imgSize.x - textSize.x) * 0.5f,
                                 p.y + (imgSize.y - textSize.y) * 0.5f);
            dl->AddText(textPos, IM_COL32(200, 200, 200, 255), msg);
        }

        ImGui::Dummy(imgSize);
    } else {
        ImGui::Image((ImTextureID)(intptr_t)texture, imgSize);
    }

    ImGui::End();
}

void Screen::setScale(float s) {
    scale = s;
}

void Screen::shutdown() {
    SDL_DestroyTexture(texture);
    texture   = nullptr;
    hasUpdate = false;
}
