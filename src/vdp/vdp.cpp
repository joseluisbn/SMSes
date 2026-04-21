// src/vdp/vdp.cpp
#include "vdp/vdp.h"
#include "memory/bus.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

VDP::VDP(Bus& bus) : bus(bus) {}

// ─────────────────────────────────────────────────────────────────────────────
// reset()
// ─────────────────────────────────────────────────────────────────────────────

void VDP::reset()
{
    vram.fill(0);
    cram.fill(0);
    regs.fill(0);

    addrReg    = 0;
    codeReg    = 0;
    firstByte  = true;
    readBuffer = 0;

    statusReg  = 0;
    irqPending = false;

    currentLine  = 0;
    cycleCounter = 0;
    lineCounter  = regs[10];
    vScroll      = 0;
    frameReady   = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// CPU-facing data port
// ─────────────────────────────────────────────────────────────────────────────

uint8_t VDP::readData()
{
    uint8_t val = readBuffer;
    readBuffer  = vram[addrReg & 0x3FFF];
    addrReg     = (addrReg + 1) & 0x3FFF;
    firstByte   = true;
    return val;
}

uint8_t VDP::readStatus()
{
    uint8_t val = statusReg;
    statusReg  &= ~0xE0u;   // clear bits 7, 6, 5 on read
    irqPending  = false;
    firstByte   = true;
    return val;
}

void VDP::writeData(uint8_t val)
{
    firstByte = true;

    if (codeReg == 3) {
        // CRAM write — SMS: 32 entries of 1 byte each, wrap at 0x1F
        cram[addrReg & 0x1F] = val;
    } else {
        // VRAM write
        vram[addrReg & 0x3FFF] = val;
    }

    readBuffer = val;
    addrReg    = (addrReg + 1) & 0x3FFF;
}

// ─────────────────────────────────────────────────────────────────────────────
// CPU-facing control port
// ─────────────────────────────────────────────────────────────────────────────

void VDP::writeControl(uint8_t val)
{
    if (firstByte) {
        // Latch low byte of address
        addrReg   = (addrReg & 0xFF00u) | val;
        firstByte = false;
    } else {
        // Second byte: set high address bits and operation code
        addrReg   = (addrReg & 0x00FFu) | (static_cast<uint16_t>(val & 0x3F) << 8);
        codeReg   = (val >> 6) & 0x03;
        firstByte = true;

        switch (codeReg) {
            case 0:
                // VRAM read — prefetch one byte
                readBuffer = vram[addrReg & 0x3FFF];
                addrReg    = (addrReg + 1) & 0x3FFF;
                break;

            case 2: {
                // Register write
                uint8_t regNum = val & 0x0F;
                uint8_t regVal = static_cast<uint8_t>(addrReg & 0xFF);
                if (regNum < 16) {
                    regs[regNum] = regVal;
                    if (regNum == 10) {
                        lineCounter = regVal;   // reload line counter immediately
                    }
                }
                break;
            }

            default:
                break;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Counter reads
// ─────────────────────────────────────────────────────────────────────────────

uint8_t VDP::getVCounter() const
{
    // NTSC: lines 0x00–0xDA are returned directly.
    // Lines 0xDB–0xFF wrap back by 6 to fit the 8-bit counter range.
    if (currentLine <= 0xDA) {
        return static_cast<uint8_t>(currentLine);
    }
    return static_cast<uint8_t>(currentLine - 6);
}

uint8_t VDP::getHCounter() const
{
    // Simplified: full implementation requires cycle-accurate H counter.
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// IRQ
// ─────────────────────────────────────────────────────────────────────────────

bool VDP::getIRQ()
{
    return irqPending;
}

void VDP::clearIRQ()
{
    irqPending = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Framebuffer
// ─────────────────────────────────────────────────────────────────────────────

const uint32_t* VDP::getFramebuffer() const
{
    return framebuffer.data();
}

bool VDP::pollFrameReady()
{
    if (frameReady) {
        frameReady = false;
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// tick() — called every CPU step with the number of cycles consumed
// ─────────────────────────────────────────────────────────────────────────────

void VDP::tick(int cpuCycles)
{
    cycleCounter += cpuCycles;

    while (cycleCounter >= CYCLES_PER_LINE) {
        cycleCounter -= CYCLES_PER_LINE;
        advanceLine();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// advanceLine() — process one complete scanline
// ─────────────────────────────────────────────────────────────────────────────

void VDP::advanceLine()
{
    // Render active display lines
    if (currentLine < SCREEN_HEIGHT) {
        renderScanline(currentLine);
    }

    // Line interrupt counter — active during display area and the line after
    if (currentLine <= SCREEN_HEIGHT) {
        lineCounter--;
        if (lineCounter < 0) {
            lineCounter = regs[10];
            if (regs[0] & 0x10) {       // R0 bit 4: line interrupt enable
                irqPending = true;
            }
        }
    }

    // VBlank starts on the first line after the active display
    if (currentLine == SCREEN_HEIGHT) {
        statusReg |= 0x80;              // set VBlank flag (bit 7)
        if (regs[1] & 0x20) {           // R1 bit 5: frame interrupt enable
            irqPending = true;
        }
        vScroll    = regs[9];           // latch vertical scroll for next frame
        frameReady = true;
    }

    // Outside active display: keep line counter reloaded
    if (currentLine >= SCREEN_HEIGHT) {
        lineCounter = regs[10];
    }

    currentLine++;
    if (currentLine >= LINES_PER_FRAME) {
        currentLine = 0;
    }
}
