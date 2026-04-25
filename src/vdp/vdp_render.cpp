// src/vdp/vdp_render.cpp
#include "vdp/vdp.h"

#include <algorithm>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// renderScanline — dispatch for one active display line
// ─────────────────────────────────────────────────────────────────────────────

void VDP::renderScanline(int line)
{
    if (!(regs[1] & 0x40)) {
        // Display disabled: fill with border color (R7 low nibble, palette 1)
        const uint32_t border = cramToRGBA(regs[7] & 0x0Fu, 1);
        std::fill(framebuffer.begin() + line * SCREEN_WIDTH,
                  framebuffer.begin() + (line + 1) * SCREEN_WIDTH, border);
        return;
    }

    renderBackground(line);
    renderSprites(line);

    // R0 bit 5: left-column mask — overwrite first 8px with border color
    if (regs[0] & 0x20) {
        const uint32_t border = cramToRGBA(regs[7] & 0x0Fu, 1);
        for (int x = 0; x < 8; x++) {
            framebuffer[line * SCREEN_WIDTH + x] = border;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// cramToRGBA — convert a CRAM color entry to 0xAARRGGBB (RGBA)
//
// SMS CRAM: 32 entries × 1 byte each, BGR222 format:
//   bits 5-4 = Red (0-3), bits 3-2 = Green (0-3), bits 1-0 = Blue (0-3)
// Scale each channel: 0→0, 1→85, 2→170, 3→255  (n * 85)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t VDP::cramToRGBA(uint8_t colorIndex, uint8_t palette)
{
    uint8_t cramIdx = palette * 16 + colorIndex;  // each palette has 16 colors
    uint8_t lo      = cram[cramIdx];               // 1 byte per color (SMS)

    uint8_t r = static_cast<uint8_t>(((lo >> 4) & 0x03) * 85);
    uint8_t g = static_cast<uint8_t>(((lo >> 2) & 0x03) * 85);
    uint8_t b = static_cast<uint8_t>(( lo        & 0x03) * 85);

    return (0xFFu << 24)
         | (static_cast<uint32_t>(r) << 16)
         | (static_cast<uint32_t>(g) <<  8)
         |  static_cast<uint32_t>(b);
}

// ─────────────────────────────────────────────────────────────────────────────
// renderBackground — Mode 4 background layer for one scanline
// ─────────────────────────────────────────────────────────────────────────────

void VDP::renderBackground(int line)
{
    const uint16_t ntBase  = static_cast<uint16_t>((regs[2] & 0x0E) << 10);
    const uint8_t  hScroll = regs[8];

    // ── Vertical scroll (latched at VBlank, stored in vScroll) ───────────────
    // Mode 4 name table height is 28 rows = 224 lines; wrap with mod 224.
    // R0 bit 7: lock the bottom 4 rows (screen lines 160–191) from v-scrolling.
    const bool vScrollLock = (regs[0] & 0x80) != 0;
    int effY;
    if (vScrollLock && line >= 160) {
        effY = line;  // no vertical scroll for bottom 4 screen rows
    } else {
        effY = (line + static_cast<int>(vScroll)) % 224;
    }
    const int row   = effY / 8;
    const int fineY = effY % 8;

    // ── Per-pixel horizontal rendering ───────────────────────────────────────
    for (int x = 0; x < SCREEN_WIDTH; x++) {

        // R0 bit 6: lock columns 0-1 of the name table (no horizontal scroll)
        const uint8_t effectiveHScroll =
            ((regs[0] & 0x40) && x < 16) ? 0 : hScroll;

        const int effX  = (x - static_cast<int>(effectiveHScroll) + 256) % 256;
        const int col   = effX / 8;
        const int fineX = effX % 8;

        // ── Name table entry (2 bytes, little-endian) ─────────────────────────
        const uint16_t entryAddr = ntBase
            + static_cast<uint16_t>((row * 32 + col) * 2);
        const uint16_t entry = static_cast<uint16_t>(vram[entryAddr])
            | (static_cast<uint16_t>(vram[entryAddr + 1]) << 8);

        const uint16_t tileIdx =  entry & 0x1FFu;
        const bool     hFlip   = (entry >>  9) & 1;
        const bool     vFlip   = (entry >> 10) & 1;
        const uint8_t  palette = (entry >> 11) & 1;
        const bool     priority = (entry >> 12) & 1;

        // ── Tile pixel extraction (4bpp planar, 32 bytes/tile) ────────────────
        // Layout: 4 consecutive bytes per row, one bit per bitplane.
        const int tileY = vFlip ? (7 - fineY) : fineY;
        const int tileX = hFlip ? (7 - fineX) : fineX;

        const uint32_t tileAddr = static_cast<uint32_t>(tileIdx) * 32
            + static_cast<uint32_t>(tileY) * 4;

        const uint8_t bitSel = static_cast<uint8_t>(7 - tileX);
        const uint8_t p0 = (vram[tileAddr + 0] >> bitSel) & 1u;
        const uint8_t p1 = (vram[tileAddr + 1] >> bitSel) & 1u;
        const uint8_t p2 = (vram[tileAddr + 2] >> bitSel) & 1u;
        const uint8_t p3 = (vram[tileAddr + 3] >> bitSel) & 1u;

        const uint8_t colorIdx = p0 | (p1 << 1) | (p2 << 2) | (p3 << 3);

        // Store raw color index for sprite priority comparison
        bgColorLine[x] = colorIdx;

        // ── Write to framebuffer ──────────────────────────────────────────────
        // Priority flag is encoded in blue channel bit 0 for the sprite
        // blending pass. Screen must mask & 0xFFFFFFFEu before display.
        uint32_t rgba = cramToRGBA(colorIdx, palette);
        if (priority) rgba |= 0x00000001u;

        framebuffer[line * SCREEN_WIDTH + x] = rgba;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// renderSprites — Mode 4 sprite layer for one scanline
// ─────────────────────────────────────────────────────────────────────────────

void VDP::renderSprites(int line)
{
    const uint16_t satBase = static_cast<uint16_t>((regs[5] & 0x7Eu) << 7);
    const bool     tall    = (regs[1] & 0x02) != 0;  // 8x16 sprites
    const int      sprH    = tall ? 16 : 8;

    // ── Collect up to 8 visible sprites for this scanline ────────────────────
    std::vector<int> visible;
    visible.reserve(8);

    for (int i = 0; i < 64; i++) {
        const uint8_t y = vram[satBase + i];
        if (y == 0xD0) break;                   // end-of-list sentinel

        const int sprY = static_cast<int>(y) + 1;
        if (line >= sprY && line < sprY + sprH) {
            visible.push_back(i);
        }
        if (static_cast<int>(visible.size()) == 8) {
            statusReg |= 0x40;                  // sprite overflow flag
            break;
        }
    }

    // ── Per-pixel drawn mask for sprite collision detection ───────────────────
    std::array<bool, SCREEN_WIDTH> spriteDrawn {};

    // ── Render back-to-front so sprite 0 wins (drawn last) ───────────────────
    for (int vi = static_cast<int>(visible.size()) - 1; vi >= 0; vi--) {
        const int     idx  = visible[vi];
        const uint8_t y    = vram[satBase + idx];
        int           xPos = static_cast<int>(vram[satBase + 0x80 + idx * 2]);
        uint8_t       tile = vram[satBase + 0x80 + idx * 2 + 1];

        // R0 bit 3: shift all sprites left 8 pixels
        if (regs[0] & 0x08) xPos -= 8;

        // Sprite pattern generator base: R6 bit 2 selects 0x0000 or 0x2000
        const uint16_t tileBase = (regs[6] & 0x04) ? 0x2000u : 0x0000u;

        const int sprY    = static_cast<int>(y) + 1;
        int       tileRow = line - sprY;

        if (tall && tileRow >= 8) {
            tile    = (tile & 0xFEu) | 1u;      // second tile of 8×16 pair
            tileRow -= 8;
        }

        const uint32_t tileAddr = tileBase
            + static_cast<uint32_t>(tile) * 32
            + static_cast<uint32_t>(tileRow) * 4;

        for (int px = 0; px < 8; px++) {
            const int screenX = xPos + px;
            if (screenX < 0 || screenX >= SCREEN_WIDTH) continue;

            const uint8_t bitSel   = static_cast<uint8_t>(7 - px);
            const uint8_t p0       = (vram[tileAddr + 0] >> bitSel) & 1u;
            const uint8_t p1       = (vram[tileAddr + 1] >> bitSel) & 1u;
            const uint8_t p2       = (vram[tileAddr + 2] >> bitSel) & 1u;
            const uint8_t p3       = (vram[tileAddr + 3] >> bitSel) & 1u;
            const uint8_t colorIdx = p0 | (p1 << 1) | (p2 << 2) | (p3 << 3);

            if (colorIdx == 0) continue;        // color 0 = transparent

            // Sprite collision detection
            if (spriteDrawn[screenX]) {
                statusReg |= 0x20;              // sprite collision flag
            }
            spriteDrawn[screenX] = true;

            // Background priority: a high-priority bg tile blocks the sprite
            // UNLESS the bg pixel at that position is transparent (color 0).
            const uint32_t bgPixel    = framebuffer[line * SCREEN_WIDTH + screenX];
            const bool     bgPriority = (bgPixel & 0x01u) != 0;
            if (bgPriority && bgColorLine[screenX] != 0) continue;

            // Sprites always use palette 1 (CRAM entries 16–31)
            framebuffer[line * SCREEN_WIDTH + screenX] = cramToRGBA(colorIdx, 1);
        }
    }
}
