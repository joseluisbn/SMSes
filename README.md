# SMS Emulator

![Build](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-green)
![Status](https://img.shields.io/badge/status-active-success)
![Last commit](https://img.shields.io/github/last-commit/joseluisbn/SMSes)
![C++](https://img.shields.io/badge/C%2B%2B-23-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-informational)

A Sega Master System emulator written in C++23.

## Status

| Phase | Component | Status |
|-------|-----------|--------|
| 1 | SDL3 + ImGui UI shell | ✅ Done |
| 2 | Z80 CPU (full opcode tables) | ✅ Done |
| 3 | Memory bus, Sega mapper, I/O controller | ✅ Done |
| 4 | VDP 315-5124 | ✅ Done |
| 5 | PSG SN76489 | ✅ Done |
| 6 | System integration, ROM execution, debugger | ✅ Done |
| 7 | Timing accuracy, save states, ROM compatibility fixes | ✅ Done |
| 8 | Testing, bugfixes | ⏳ Planned |

## Features

### Emulation
- Full Z80 instruction set including all undocumented opcodes (CB, ED, DD/FD, DDCB/FDCB prefixes) and undocumented flags (F3/F5)
- EI instruction delay: interrupts are suppressed for one instruction after EI, matching hardware behaviour
- VDP 315-5124: Mode 4 background, sprites, NTSC/PAL timing, H/V counters, line interrupts, vertical and horizontal scroll with per-row lock (R0 bits 6–7)
- PSG SN76489: 3 tone channels + noise channel with LFSR, SDL3 audio stream output
- Sega mapper: 16 KB bank switching, cartridge RAM support, correct modulo handling for ROMs smaller than 3 pages (16 KB / 32 KB)
- Cycle-accurate VDP wait states during active display; accumulator-based frame timing capped at 100 ms to prevent spiral of death after sleep/resume

### User interface
- Drag-and-drop ROM loading; auto-detects NTSC/PAL from header
- 1×/2×/3× display scale
- Turbo (uncapped) mode
- Save states: 10 slots, F5 to save, F8 to load; slot selector and menu items in the File menu
- Save files written to `saves/slot_N.sms` relative to the working directory

### Debugger (ImGui panels, toggle from Debug menu)
- **CPU Registers** — live Z80 register and flags view, step mode
- **Disassembler** — Z80 disassembly with breakpoint support
- **Memory Viewer** — RAM, VRAM and ROM hex views
- **VDP Viewer** — register dump, palette, nametable info
- **PSG Monitor** — per-channel waveform history and volume meters

## Controls

| Key | Action |
|-----|--------|
| Arrow keys | D-pad |
| Z | Button 1 |
| X | Button 2 |
| F5 | Save state (current slot) |
| F8 | Load state (current slot) |
| Escape | Quit |

## Requirements

- **CMake** 3.25 or later
- **Ninja** (recommended build system)
- **vcpkg** (for dependency management)
- A C++23-capable compiler: MSVC 19.38+, GCC 13+, or Clang 17+

## Dependencies

Managed via vcpkg (baseline `256acc64`):

| Library | Version |
|---------|---------|
| SDL3 | 3.4.4 |
| Dear ImGui (sdl3-binding + sdl3-renderer-binding) | 1.92.7 |
| GoogleTest | 1.17.0 |

## Building

### 1. Install vcpkg

```sh
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && bootstrap-vcpkg.bat   # Windows
# or
cd vcpkg && ./bootstrap-vcpkg.sh  # Linux / macOS
```

Set the `VCPKG_ROOT` environment variable to the vcpkg directory.

### 2. Configure & build

```sh
# Debug build
cmake --preset debug
cmake --build --preset debug

# Release build
cmake --preset release
cmake --build --preset release
```

### 3. Run the emulator

```sh
./build/debug/sms-emulator      # Windows: build\debug\sms-emulator.exe
```

Drag and drop a `.sms` ROM file onto the window to load it.

### 4. Run the tests

```sh
ctest --preset debug --output-on-failure
```

## Project Structure

```
SMSes/
├── src/
│   ├── main.cpp
│   ├── cpu/
│   │   ├── z80.h / z80.cpp              — Z80 registers, flags, core helpers
│   │   ├── z80_opcodes.cpp              — Main opcode table (0x00–0xFF)
│   │   ├── z80_cb_opcodes.cpp           — CB prefix (bit ops)
│   │   ├── z80_ed_opcodes.cpp           — ED prefix (block transfers, 16-bit ops)
│   │   └── z80_ddfd_opcodes.cpp         — DD/FD prefix (IX/IY indexed)
│   ├── memory/
│   │   ├── bus.h / bus.cpp              — System bus (owns Mapper + IO)
│   │   ├── mapper.h / mapper.cpp        — Sega bank mapper
│   ├── io/
│   │   └── io.h / io.cpp                — I/O port controller (joypad, VDP, PSG)
│   ├── vdp/
│   │   ├── vdp.h / vdp.cpp              — VDP registers, timing, counters
│   │   └── vdp_render.cpp               — Mode 4 background and sprite renderer
│   ├── psg/
│   │   └── psg.h / psg.cpp              — SN76489 tone/noise synthesis
│   ├── system/
│   │   ├── sms.h / sms.cpp              — Top-level system orchestrator
│   │   └── save_state.h                 — POD save-state structs (binary serialisable)
│   └── ui/
│       ├── app.h / app.cpp              — Main application loop, frame timing, audio
│       ├── menubar.h / menubar.cpp      — ImGui menu bar (file, emulation, debug, view)
│       ├── screen.h / screen.cpp        — SDL3 texture display
│       └── debugger/
│           ├── debugger.h / debugger.cpp       — Debugger shell
│           ├── cpu_panel.h / cpu_panel.cpp     — Z80 registers + step mode
│           ├── disasm_panel.h / disasm_panel.cpp — Disassembler + breakpoints
│           ├── memory_panel.h / memory_panel.cpp — Hex viewer (RAM/VRAM/ROM)
│           ├── vdp_panel.h / vdp_panel.cpp     — VDP register viewer
│           └── psg_panel.h / psg_panel.cpp     — PSG channel monitor
├── tests/
│   ├── mock_bus.h                       — Minimal Bus stub for unit tests
│   ├── test_z80.cpp                     — Z80 unit tests (GoogleTest)
│   └── test_vdp.cpp                     — VDP unit tests (GoogleTest)
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json
```

## CPU Implementation Notes

- `Flags` is a union with named bitfields (`S`, `Z`, `H`, `PV`, `N`, `C`) and a `uint8_t raw` member for bulk access.
- All undocumented flags (F3/F5) are updated per the Z80 undocumented behaviour specification.
- DD/FD prefixes share a single `executeIndexed()` handler; DDCB/FDCB share `executeIndexedCB()`.
- `Bus` is injected by reference into the Z80 at construction — no global state.
- EI sets `eiDelay = true`; `step()` checks this flag before allowing IRQ acknowledgement, ensuring the instruction immediately after EI is non-interruptible.

## Save State Format

Save states are raw binary dumps of a fixed-layout `SaveState` struct (defined in `system/save_state.h`):

| Field | Size | Notes |
|-------|------|-------|
| Magic (`0x534D5353`) | 4 B | ASCII "SMSS" |
| Version (`1`) | 4 B | Bumped on breaking layout changes |
| Z80 registers | — | All main, shadow, index, special regs |
| VDP state | ~16.4 KB | VRAM, CRAM, registers, counters |
| PSG state | — | Tone channels, noise channel, latch |
| Mapper state | ~16 KB | RAM, cart RAM, bank registers |
| Region | 1 B | `0` = NTSC, `1` = PAL |

## Memory Map

| Address range | Description |
|---------------|-------------|
| `0x0000–0x03FF` | ROM page 0 (always fixed — boot vectors) |
| `0x0400–0x3FFF` | ROM slot 0 (banked via mapper reg 0xFFFD) |
| `0x4000–0x7FFF` | ROM slot 1 (banked via mapper reg 0xFFFE) |
| `0x8000–0xBFFF` | ROM slot 2 / cartridge RAM (mapper reg 0xFFFF) |
| `0xC000–0xDFFF` | System RAM (8 KB) |
| `0xE000–0xFFFF` | System RAM mirror |
| `0xFFFC–0xFFFF` | Mapper registers (write) |

## I/O Port Map

| Port | Direction | Description |
|------|-----------|-------------|
| `0x3F` | Write | I/O control register |
| `0x7E` | Read | VDP V-counter |
| `0x7F` | Read | VDP H-counter |
| `0x7E–0x7F` | Write | PSG data |
| `0xBE` | Read/Write | VDP data port |
| `0xBF` | Read/Write | VDP control port |
| `0xC0` / `0xDC` | Read | Joypad 1 + joypad 2 lower bits |
| `0xC1` / `0xDD` | Read | Joypad 2 upper bits |
