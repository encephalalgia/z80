#include <gtest/gtest.h>
#include "z80.h"

class z80Test : public testing::Test {
protected:
    struct memory {
        int ram[65336] {
            0x04
        };

        [[nodiscard]] u8 read8(const u16 addr) const { return ram[addr]; }
        [[nodiscard]] u16 read16(const u16 addr) const { return ram[addr] << 8 | ram[addr + 1]; }
        void write(const u16 addr, const u8 val) { ram[addr] = val; }
        void write(const u16 addr, const u16 val) { ram[addr] = val >> 8; ram[addr + 1] = val & 0xFF; }
    };

    struct io {
        u8 port[256] {};

        [[nodiscard]] u8 input(const u8 addr) const { return port[addr]; }
        void output(const u8 addr, const u8 val) { port[addr] = val; }
    };

    z80<memory, io> z80_ {};
};

TEST_F(z80Test, jsmooUnitTests)
{
    /*
    https://github.com/raddad772/jsmoo/tree/main/misc/tests/GeneratedTests/z80
    load test .json;
    for test in test.json:
        set initial processor state from test;
        set initial RAM state from test;

        for cycle in test:
            cycle processor
            if we are checking cycle-by-cycle:
                compare our R/W/MRQ/IO/Address/Data pins against the current cycle;

        compare final RAM state to test and report any errors;
        compare final processor state to test and report any errors;
     */

    z80_.regs.b = 0x7F;
    z80_.run(4);
    EXPECT_EQ(z80_.flags.pf, 0b100);
}

TEST_F(z80Test, prelim)
{

}

TEST_F(z80Test, zexall)
{

}

TEST_F(z80Test, zexdoc)
{

}