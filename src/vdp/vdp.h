// src/vdp/vdp.h
#pragma once

#include <array>
#include <cstdint>

class Bus;

// ---------------------------------------------------------------------------
// Region selection — controls frame length, clock frequency, and VCounter
// ---------------------------------------------------------------------------
enum class Region { NTSC, PAL };

class VDP {
public:
    // -------------------------------------------------------------------------
    // Timing constants
    // -------------------------------------------------------------------------
    static constexpr int    SCREEN_WIDTH        = 256;
    static constexpr int    SCREEN_HEIGHT       = 192;
    static constexpr int    CYCLES_PER_LINE     = 228;   // CPU cycles per scanline (both regions)
    static constexpr int    VBLANK_START_LINE   = 192;
    static constexpr int    ACTIVE_DISPLAY_AREA = 192;

    static constexpr int    NTSC_LINES          = 262;
    static constexpr int    PAL_LINES           = 313;

    static constexpr double NTSC_CLOCK_HZ       = 3579545.0;  // ~3.58 MHz
    static constexpr double PAL_CLOCK_HZ        = 3546895.0;  // ~3.55 MHz

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------
    explicit VDP(Bus& bus);

    void reset();
    void tick(int cpuCycles);   // advance VDP state by cpuCycles

    // -------------------------------------------------------------------------
    // Region
    // -------------------------------------------------------------------------
    void     setRegion(Region r);
    Region   getRegion()       const;
    int      getLinesPerFrame() const;
    double   getClockHz()      const;

    // -------------------------------------------------------------------------
    // IRQ
    // -------------------------------------------------------------------------
    bool getIRQ();
    void clearIRQ();

    // -------------------------------------------------------------------------
    // CPU-facing I/O ports
    // -------------------------------------------------------------------------
    uint8_t readData();
    uint8_t readStatus();
    void    writeData(uint8_t val);
    void    writeControl(uint8_t val);

    // -------------------------------------------------------------------------
    // Counter reads (I/O ports 0x40 / 0x41)
    // -------------------------------------------------------------------------
    //
    // VCounter tables (from Sega Master System Technical Reference):
    //
    // NTSC (262 lines):
    //   Scanlines 0x00–0xDA (0–218)   → VCounter = line
    //   Scanlines 0xDB–0xFF (219–261) → VCounter = line - 6
    //   (0xDB maps to 0xD5, continues down to 0xFF→0xF9)
    //
    // PAL (313 lines):
    //   Scanlines 0x00–0xF2 (0–242)   → VCounter = line
    //   Scanlines 0xF3–0xFF (243–255) → VCounter = line - 57   (0xF3→0xBA)
    //   Scanlines 0x100–0x138 (256–312) wrap: VCounter continues 0xBA→0xFF
    //   i.e. for line >= 256: VCounter = line - 256 + 0xBA
    //        combined formula: VCounter = (line - 57) & 0xFF  for line >= 0xF3
    //
    uint8_t getVCounter() const;
    uint8_t getHCounter() const;

    // -------------------------------------------------------------------------
    // Framebuffer access (consumed by Screen)
    // -------------------------------------------------------------------------
    const uint32_t* getFramebuffer() const;
    bool            pollFrameReady();   // returns true once per completed frame

    // -------------------------------------------------------------------------
    // Debugger access (inline helpers)
    // -------------------------------------------------------------------------
    const uint8_t* getVRAM()        const { return vram.data(); }
    const uint8_t* getCRAM()        const { return cram.data(); }
    const uint8_t* getRegs()        const { return regs.data(); }
    int            getCurrentLine() const { return currentLine; }

private:
    Bus& bus;

    // -------------------------------------------------------------------------
    // Memory
    // -------------------------------------------------------------------------
    std::array<uint8_t, 16384> vram {};   // 16 KiB Video RAM
    std::array<uint8_t,    32> cram {};   // 32 colors × 1 byte each (SMS BGR222)
    std::array<uint8_t,    16> regs {};   // VDP registers R0–R10 (+ padding)

    // -------------------------------------------------------------------------
    // Control port state machine
    // -------------------------------------------------------------------------
    uint16_t addrReg   = 0;     // VRAM address register (14-bit)
    uint8_t  codeReg   = 0;     // 2-bit code: 0=VRAM rd, 1=VRAM wr, 2=REG wr, 3=CRAM wr
    bool     firstByte = true;  // control-port write latch (false after 1st byte)
    uint8_t  readBuffer = 0;    // prefetch buffer for VRAM data reads

    // -------------------------------------------------------------------------
    // Status register
    //   bit 7 : Frame interrupt pending (VBlank)
    //   bit 6 : Sprite overflow (>8 sprites on a line)
    //   bit 5 : Sprite collision
    // -------------------------------------------------------------------------
    uint8_t statusReg = 0;

    // -------------------------------------------------------------------------
    // IRQ line
    // -------------------------------------------------------------------------
    bool irqPending = false;

    // -------------------------------------------------------------------------
    // Counters
    // -------------------------------------------------------------------------
    int     currentLine  = 0;   // current scanline (0–261 NTSC)
    int     cycleCounter = 0;   // CPU cycles accumulated in the current line
    int     lineCounter  = 0;   // R10 countdown — triggers line IRQ on underflow
    uint8_t vScroll      = 0;   // vertical scroll latched at frame start

    // -------------------------------------------------------------------------
    // Region
    // -------------------------------------------------------------------------
    Region region       = Region::NTSC;
    int    linesPerFrame = NTSC_LINES;  // updated by setRegion()

    // -------------------------------------------------------------------------
    // Framebuffer
    // -------------------------------------------------------------------------
    // Sized for PAL (largest frame) so no reallocation is needed on region switch.
    std::array<uint32_t, SCREEN_WIDTH * PAL_LINES> framebuffer {};
    bool frameReady = false;

    // Per-scanline background color index (used by sprite priority logic)
    std::array<uint8_t, SCREEN_WIDTH> bgColorLine {};

    // -------------------------------------------------------------------------
    // Internal rendering helpers
    // -------------------------------------------------------------------------
    void     renderScanline(int line);
    void     renderBackground(int line);
    void     renderSprites(int line);
    uint32_t cramToRGBA(uint8_t colorIndex, uint8_t palette);
    void     advanceLine();
};
