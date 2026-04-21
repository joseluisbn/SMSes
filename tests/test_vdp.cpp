// tests/test_vdp.cpp
#include "vdp/vdp.h"
#include "mock_bus.h"

#include <gtest/gtest.h>

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class VDPTest : public ::testing::Test {
protected:
    MockBus bus;
    VDP     vdp{bus};

    void SetUp() override { vdp.reset(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(VDPTest, reset_clears_status)
{
    // After reset, readStatus() should return 0
    EXPECT_EQ(vdp.readStatus(), 0x00);
}

TEST_F(VDPTest, writeControl_latches_address)
{
    // Write 0x34 then 0x7E → address = 0x3E34, code = 1 (VRAM write)
    vdp.writeControl(0x34);
    vdp.writeControl(0x7E);

    // Write a byte and verify it lands in VRAM at 0x3E34
    vdp.writeData(0xAB);

    EXPECT_EQ(vdp.getVRAM()[0x3E34], 0xAB);
}

TEST_F(VDPTest, writeControl_register_write)
{
    // Second byte: code = 10b, reg = 1 → R1 = 0xFF
    // First  byte carries regVal (low 8 bits of addrReg)
    vdp.writeControl(0xFF);   // low byte → regVal = 0xFF
    vdp.writeControl(0x81);   // code = 10, regNum = 1

    EXPECT_EQ(vdp.getRegs()[1], 0xFF);
}

TEST_F(VDPTest, CRAM_write_via_control_port)
{
    // code = 11b = CRAM write
    vdp.writeControl(0x00);   // address = 0
    vdp.writeControl(0xC0);   // code = 11

    vdp.writeData(0x3F);      // white in SMS BGR222 palette

    EXPECT_EQ(vdp.getCRAM()[0], 0x3F);
}

TEST_F(VDPTest, vblank_irq_fires_at_line_192)
{
    // Enable frame IRQ: R1 bit 5
    vdp.writeControl(0x20);   // regVal = 0x20
    vdp.writeControl(0x81);   // code = 10, R1 = 0x20

    // VBlank fires when advanceLine() runs with currentLine==192.
    // tick(192*CPL) processes lines 0-191 (192 calls) → currentLine=192, no IRQ yet.
    // One more line is needed to trigger the VBlank block.
    vdp.tick(193 * VDP::CYCLES_PER_LINE);

    EXPECT_TRUE(vdp.getIRQ());
}

TEST_F(VDPTest, vcounter_wraps_after_line_DA)
{
    // Tick to line 0xDB (219 decimal)
    vdp.tick(0xDB * VDP::CYCLES_PER_LINE);

    // NTSC VCounter: lines > 0xDA are reported as line - 6
    EXPECT_EQ(vdp.getVCounter(), static_cast<uint8_t>(0xDB - 6));
}

TEST_F(VDPTest, pollFrameReady_true_once_per_frame)
{
    // Tick a full frame
    vdp.tick(VDP::LINES_PER_FRAME * VDP::CYCLES_PER_LINE);

    EXPECT_TRUE(vdp.pollFrameReady());
    EXPECT_FALSE(vdp.pollFrameReady());  // flag clears after first poll
}
