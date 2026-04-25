// src/system/sms.cpp
#include "system/sms.h"
#include "system/save_state.h"
#include <fstream>

// ── Constructor ───────────────────────────────────────────────────────────────

SMS::SMS()
    : vdp(),
      psg(),
      bus(vdp, psg),
      cpu(bus)
{
    computeCyclesPerFrame();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void SMS::computeCyclesPerFrame()
{
    // NTSC: 262 lines * 228 cycles = 59736 cycles/frame
    // PAL:  313 lines * 228 cycles = 71364 cycles/frame
    cyclesPerFrame = vdp.getLinesPerFrame() * VDP::CYCLES_PER_LINE;
}

// ── Region ────────────────────────────────────────────────────────────────────

void SMS::setRegion(Region r)
{
    region = r;
    vdp.setRegion(r);
    psg.setClockHz(vdp.getClockHz());
    computeCyclesPerFrame();
}

Region SMS::getRegion() const
{
    return region;
}

// ── ROM loading ───────────────────────────────────────────────────────────────

bool SMS::loadROM(const std::vector<uint8_t>& data)
{
    // Auto-detect region from SMS ROM header before loading
    if (data.size() >= 0x8000) {
        const char magic[8] = { 'T', 'M', 'R', ' ', 'S', 'E', 'G', 'A' };
        bool hasMagic = true;
        for (int i = 0; i < 8; ++i) {
            if (data[0x7FF0 + i] != static_cast<uint8_t>(magic[i])) {
                hasMagic = false;
                break;
            }
        }
        if (hasMagic) {
            const uint8_t code     = (data[0x7FFF] >> 4) & 0x0F;
            const Region  detected = (code == 3 || code == 5) ? Region::NTSC : Region::PAL;
            setRegion(detected);
        }
    }

    if (!bus.loadROM(data))
        return false;

    romLoaded = true;
    return true;
}

bool SMS::isROMLoaded() const
{
    return romLoaded;
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void SMS::reset()
{
    cpu.reset();
    vdp.reset();
    psg.reset();
    bus.reset();
    // romLoaded intentionally preserved — a hardware reset does not eject the cartridge
}

// ── Frame execution ───────────────────────────────────────────────────────────

void SMS::runFrame()
{
    if (!romLoaded || paused)
        return;

    int cyclesRun = 0;

    while (cyclesRun < cyclesPerFrame) {
        // 1. Execute one CPU instruction
        int cycles = cpu.step();

        // 2. Apply VDP wait state if the instruction accessed a VDP I/O port
        //    during active display (not HBlank or VBlank).
        if (bus.wasLastIOVDP())
            cycles += vdp.getWaitCycles();

        cyclesRun += cycles;

        // 3. Advance VDP by the (possibly extended) cycle count
        vdp.tick(cycles);

        // 4. Forward any pending VDP IRQ to the CPU
        if (vdp.getIRQ()) {
            vdp.clearIRQ();
            cpu.irq();
        }
    }

    // Audio is generated separately via fillAudio().
    // PSG is clocked inside generateSamples(), not here.
}

// ── Audio ─────────────────────────────────────────────────────────────────────

void SMS::fillAudio(float* buffer, int numSamples)
{
    psg.generateSamples(buffer, numSamples);
}

double SMS::targetFPS() const
{
    return (region == Region::NTSC) ? 59.9227 : 49.7016;
}

// ── Input ─────────────────────────────────────────────────────────────────────

void SMS::setJoypad1(uint8_t state)
{
    bus.getIO().setJoypad1(state);
}

void SMS::setJoypad2(uint8_t state)
{
    bus.getIO().setJoypad2(state);
}

// ── Component accessors ───────────────────────────────────────────────────────

VDP&       SMS::getVDP()       { return vdp; }
PSG&       SMS::getPSG()       { return psg; }
Bus&       SMS::getBus()       { return bus; }
Z80&       SMS::getCPU()       { return cpu; }

const VDP& SMS::getVDP() const { return vdp; }
const PSG& SMS::getPSG() const { return psg; }
const Bus& SMS::getBus() const { return bus; }
const Z80& SMS::getCPU() const { return cpu; }

// ── Save states ───────────────────────────────────────────────────────────────────────

SaveState SMS::saveState() const
{
    SaveState s;
    s.cpu    = cpu.getRegisters();
    s.vdp    = vdp.captureState();
    s.psg    = psg.captureState();
    s.mapper = bus.getMapper().captureState();
    s.region = static_cast<uint8_t>(region);
    return s;
}

bool SMS::loadSaveState(const SaveState& s)
{
    if (s.magic   != SaveState::MAGIC)   return false;
    if (s.version != SaveState::VERSION) return false;

    cpu.loadRegisters(s.cpu);
    vdp.loadState(s.vdp);
    psg.loadState(s.psg);
    bus.getMapper().loadState(s.mapper);
    setRegion(static_cast<Region>(s.region));
    return true;
}

bool SMS::saveToFile(const std::string& path) const
{
    const SaveState s = saveState();
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(&s), sizeof(s));
    return f.good();
}

bool SMS::loadFromFile(const std::string& path)
{
    SaveState s;
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(&s), sizeof(s));
    if (!f.good()) return false;
    return loadSaveState(s);
}
