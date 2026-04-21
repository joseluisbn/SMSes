// tests/test_z80.cpp
#include "cpu/z80.h"
#include "mock_bus.h"

#include <gtest/gtest.h>

// ── Test fixture ──────────────────────────────────────────────────────────────
class Z80Test : public ::testing::Test {
protected:
    MockBus bus;
    Z80     cpu{bus};

    void SetUp() override { cpu.reset(); }

    void loadROM(std::vector<uint8_t> bytes) { bus.load(bytes); }
};

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

// NOP
TEST_F(Z80Test, NOP_returns_4_cycles) {
    loadROM({0x00}); // NOP
    EXPECT_EQ(cpu.step(), 4);
    EXPECT_EQ(cpu.getRegisters().PC, 1);
}

// LD r,n
TEST_F(Z80Test, LD_B_n) {
    loadROM({0x06, 0x42}); // LD B, 0x42
    cpu.step();
    EXPECT_EQ(cpu.getRegisters().B, 0x42);
}

// INC B — flags
TEST_F(Z80Test, INC_B_sets_zero_flag_on_overflow) {
    loadROM({0x06, 0xFF, 0x04}); // LD B,0xFF; INC B
    cpu.step(); cpu.step();
    EXPECT_EQ(cpu.getRegisters().B, 0x00);
    EXPECT_TRUE(cpu.getRegisters().F.raw & 0x40u); // Z flag (bit 6)
}

// ADD A,r — carry + zero
TEST_F(Z80Test, ADD_A_B_sets_carry) {
    loadROM({0x3E, 0xFF, 0x06, 0x01, 0x80}); // LD A,0xFF; LD B,0x01; ADD A,B
    cpu.step(); cpu.step(); cpu.step();
    EXPECT_EQ(cpu.getRegisters().A, 0x00);
    EXPECT_TRUE(cpu.getRegisters().F.raw & 0x01u); // C flag (bit 0)
    EXPECT_TRUE(cpu.getRegisters().F.raw & 0x40u); // Z flag (bit 6)
}

// SUB — overflow (0x80 - 0x01 = 0x7F, signed overflow)
TEST_F(Z80Test, SUB_sets_overflow_flag) {
    loadROM({0x3E, 0x80, 0xD6, 0x01}); // LD A,0x80; SUB 0x01
    cpu.step(); cpu.step();
    EXPECT_EQ(cpu.getRegisters().A, 0x7F);
    EXPECT_TRUE(cpu.getRegisters().F.raw & 0x04u); // PV flag (bit 2)
}

// PUSH/POP roundtrip
TEST_F(Z80Test, PUSH_POP_BC_roundtrip) {
    // LD BC,0xABCD; PUSH BC; LD BC,0x0000; POP BC
    loadROM({0x01, 0xCD, 0xAB, 0xC5, 0x01, 0x00, 0x00, 0xC1});
    cpu.step(); cpu.step(); cpu.step(); cpu.step();
    EXPECT_EQ(cpu.getRegisters().BC, 0xABCDu);
}

// DJNZ — loop count
TEST_F(Z80Test, DJNZ_loops_correct_times) {
    // LD B,3; DJNZ -2  (0xFE as int8_t = -2, loops back to DJNZ itself)
    loadROM({0x06, 0x03, 0x10, 0xFE});
    cpu.step(); // LD B,3
    int iterations = 0;
    while (cpu.getRegisters().B != 0) {
        cpu.step();
        ++iterations;
    }
    EXPECT_EQ(iterations, 3);
}

// LDI block copy
TEST_F(Z80Test, LDI_copies_byte_and_decrements_BC) {
    // LD HL,0x1000 ; LD DE,0x2000 ; LD BC,0x0001 ; LDI
    loadROM({
        0x21, 0x00, 0x10,  // LD HL, 0x1000
        0x11, 0x00, 0x20,  // LD DE, 0x2000
        0x01, 0x01, 0x00,  // LD BC, 0x0001
        0xED, 0xA0         // LDI
    });
    bus.loadAt(0x1000, {0xAA}); // source byte
    cpu.step(); cpu.step(); cpu.step(); // LD HL, LD DE, LD BC
    cpu.step();                          // LDI

    EXPECT_EQ(bus.memory[0x2000], 0xAAu);
    EXPECT_EQ(cpu.getRegisters().BC, 0x0000u);
    EXPECT_FALSE(cpu.getRegisters().F.raw & 0x04u); // PV=0 (BC became 0)
}

// JR NZ timing — taken branch (12 cycles)
// Note: JR NZ tests the Z flag, not the B register.
// INC L: L starts at 0 after reset → L=1, Z=0 → JR NZ is taken.
TEST_F(Z80Test, JR_NZ_taken_costs_12_cycles) {
    loadROM({
        0x2C,        // INC L  (L=0→1, sets Z=0)
        0x20, 0x00   // JR NZ, +0  (Z=0 → taken, 12 T-states)
    });
    cpu.step(); // INC L — clears Z
    EXPECT_EQ(cpu.step(), 12);
}

// JR NZ timing — not-taken branch (7 cycles)
// XOR A: A=0xFF^0xFF=0, sets Z=1 → JR NZ is not taken.
TEST_F(Z80Test, JR_NZ_not_taken_costs_7_cycles) {
    loadROM({
        0xAF,        // XOR A  (A=0, sets Z=1)
        0x20, 0x00   // JR NZ, +0  (Z=1 → not taken, 7 T-states)
    });
    cpu.step(); // XOR A — sets Z
    EXPECT_EQ(cpu.step(), 7);
}
