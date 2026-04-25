# SMS Emulator

A work-in-progress Sega Master System emulator written in C++23.

## Status

| Phase | Component | Status |
|-------|-----------|--------|
| 1 | SDL3 + ImGui UI shell | ✅ Done |
| 2 | Z80 CPU (full opcode tables) | ✅ Done |
| 3 | Memory bus, Sega mapper, I/O controller | ✅ Done |
| 4 | VDP (video display processor) | ✅ Done |
| 5 | PSG (SN76489 sound) | ✅ Done |
| 6 | System integration & ROM execution | ⏳ Planned |

## Features

- Full Z80 instruction set including undocumented opcodes (CB, ED, DD/FD, DDCB/FDCB prefixes)
- Sega mapper with 16 KB bank switching, cartridge RAM support
- Active-low joypad input with drag-and-drop ROM loading
- ImGui-based menu bar (scale control, ROM open, reset)
- GoogleTest unit tests for the CPU

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
# Debug build (Ninja + vcpkg toolchain wired in via CMakePresets.json)
cmake --preset debug
cmake --build --preset debug

# Release build
cmake --preset release
cmake --build --preset release
```

### 3. Run the emulator

```sh
./build/debug/sms-emulator
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
│   │   ├── io.h / io.cpp                — I/O port controller (joypad, VDP, PSG)
│   └── ui/
│       ├── app.h / app.cpp              — Main application loop
│       ├── menubar.h / menubar.cpp      — ImGui menu bar
│       └── screen.h / screen.cpp        — SDL3 texture display
├── tests/
│   └── test_z80.cpp                     — Z80 unit tests (GoogleTest)
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json
```

## CPU Implementation Notes

- `Flags` is a union with named bitfields (`S`, `Z`, `H`, `PV`, `N`, `C`) and a `uint8_t raw` member.
- All undocumented flags (F3/F5) are updated per the Z80 undocumented behaviour specification.
- DD/FD prefixes share a single `executeIndexed()` handler; DDCB/FDCB share `executeIndexedCB()`.
- `Bus` is injected into the Z80 at construction — no global state.

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
| `0x40` | Read | VDP V-counter |
| `0x41` | Read | VDP H-counter |
| `0x7E–0x7F` | Write | PSG data |
| `0xBE` | Read/Write | VDP data port |
| `0xBF` | Read/Write | VDP control port |
| `0xC0` / `0xDC` | Read | Joypad 1 + joypad 2 lower bits |
| `0xC1` / `0xDD` | Read | Joypad 2 upper bits |