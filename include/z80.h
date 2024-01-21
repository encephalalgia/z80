#ifndef Z80_LIBRARY_H
#define Z80_LIBRARY_H

#include <cstdint>
#include <bit>
#include <algorithm>
#include <array>
#include <string>
#include <format>

/**
 * \brief
 * \tparam Memory
 */
template<typename Memory>
class z80 {
public:
    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using s8 = std::int8_t;

    struct Registers {
        u8 b {}, c {}, d {}, e {}, h {}, l {};
    };

    struct Flags {
        u8 sf {sbit}, zf {zbit}, yf {ybit}, hf {hbit}, xf {xbit}, pf {pbit}, nf {nbit}, cf {cbit};
    };

    /**
     * \brief
     * \param cycles
     * \return
     */
    int run(int cycles);

    /**
     * \brief
     */
    void step();

    /**
     * \brief
     */
    void reset();

    /**
     * \brief
     */
    void reqNmi();

    /**
     * \brief
     * \param vector
     */
    void reqInt(u8 vector);

    /**
     * \brief
     * \param cycle
     * \return
     */
    [[nodiscard]] std::string toString(unsigned long long cycle=0) const;

    /**
     * \brief Flags are stored separetly, to get them as a single status byte they must be or'd together.
     * \return The flag register as a single byte.
     */
    [[nodiscard]] u8 getf() const { return flags.sf | flags.zf | flags.yf | flags.hf | flags.xf | flags.pf | flags.nf | flags.cf; }

    /**
     * \brief Member function for easy setting of the flag register F.
     * \param f the Flags struct to set
     * \param val a byte representing the value of all 8 flags
     */
    void setf(Flags& f, u8 val);

    /**
     * \brief To use the CPU it must be provided a memory object. Requires 6 member functions with the following signatures:
     *  - u8 read8(u16 addr)
     *  - void write8(u16 addr, u8 val)
     *  - u16 read16(u16 addr)
     *  - void write16(u16 addr, u16 val)
     *  - u8 input(z80<Memory>* cpu, u8 port)
     *  - void output(z80<Memory>* cpu, u8 port, u8 val)
     */
    Memory memory {};

    // internals
    Registers regs {}; // 8-bit registers b,c,d,e,h,l
    Flags flags {}; // functions as the 8-bit flag register
    u16 pc {}, sp {0xFFFF}, ix {}, iy {}, wz {}; // 16-bit registers
    u8 i {}, r {}, a {0xFF}; // interrupt, refresh, accumulator
    bool iff1 {false}, iff2 {false}; // interrupt latch
    bool halted {false}; // determines if the pc is halted or not, if in halted state cpu will execute NOPs
    u8 imode {0}; // interrupt mode initialized to im0
    int requested {}; // holds the number of requested cycles, stepping the cpu subtracts from this number
    bool flagsWereModified {false}; // keeps track of whether last instruction modified the flags for use with scf and ccf
private:
    static constexpr u8 sbit {0b10000000}; // bit 7: sign flag
    static constexpr u8 zbit {0b01000000}; // bit 6: zero flag
    static constexpr u8 ybit {0b00100000}; // bit 5: undocumented flag (copy of bit 5 of the result)
    static constexpr u8 hbit {0b00010000}; // bit 4: half-carry flag
    static constexpr u8 xbit {0b00001000}; // bit 3: undocumented flag (copy of bit 3 of the result)
    static constexpr u8 pbit {0b00000100}; // bit 2: parity flag
    static constexpr u8 nbit {0b00000010}; // bit 1: add/subtract flag
    static constexpr u8 cbit {0b00000001}; // bit 0: carry flag

    // Cycle Tables
    static constexpr u8 cycles[256] {
        4, 10, 7, 6, 4, 4, 7, 4, 4, 11, 7, 6, 4, 4, 7, 4,
        8, 10, 7, 6, 4, 4, 7, 4, 12, 11, 7, 6, 4, 4, 7, 4,
        7, 10, 16, 6, 4, 4, 7, 4, 7, 11, 16, 6, 4, 4, 7, 4,
        7, 10, 13, 6, 11, 11, 10, 4, 7, 11, 13, 6, 4, 4, 7, 4,
        4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
        4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
        4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
        7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4,
        4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
        4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
        4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
        4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
        5, 10, 10, 10, 10, 11, 7, 11, 5, 10, 10, 0, 10, 17, 7, 11,
        5, 10, 10, 11, 10, 11, 7, 11, 5, 4, 10, 11, 10, 0, 7, 11,
        5, 10, 10, 19, 10, 11, 7, 11, 5, 4, 10, 4, 10, 0, 7, 11,
        5, 10, 10, 4, 10, 11, 7, 11, 5, 6, 10, 4, 10, 0, 7, 11,
    };

    static constexpr u8 bitCycles[256] {
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8,
        8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8,
        8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8,
        8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
        8, 8, 8, 8, 8, 8, 15, 8, 8, 8, 8, 8, 8, 8, 15, 8,
    };

    static constexpr u8 miscCycles[256] {
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        12, 12, 15, 20, 8, 14, 8, 9, 12, 12, 15, 20, 8, 14, 4, 9,
        12, 12, 15, 20, 8, 14, 8, 9, 12, 12, 15, 20, 8, 14, 8, 9,
        12, 12, 15, 20, 8, 14, 4, 18, 12, 12, 15, 20, 8, 14, 4, 18,
        12, 12, 15, 20, 8, 14, 4, 4, 12, 12, 15, 20, 8, 14, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        16, 16, 16, 16, 4, 4, 4, 4, 16, 16, 16, 16, 4, 4, 4, 4,
        16, 16, 16, 16, 4, 4, 4, 4, 16, 16, 16, 16, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    };

    static constexpr u8 indexCycles[256] {
        4, 14, 11, 10, 8, 8, 11, 4, 4, 15, 4, 4, 8, 8, 11, 4,
        4, 4, 4, 4, 8, 8, 11, 4, 4, 15, 4, 4, 8, 8, 11, 4,
        4, 14, 20, 10, 8, 8, 11, 4, 4, 15, 20, 10, 8, 8, 11, 4,
        4, 4, 4, 4, 23, 23, 19, 4, 4, 15, 4, 4, 8, 8, 11, 4,
        8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
        8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
        8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
        19, 19, 19, 19, 19, 19, 4, 19, 8, 8, 8, 8, 8, 8, 19, 8,
        8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
        8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
        8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
        8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 14, 4, 23, 4, 15, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 10, 4, 4, 4, 4, 4, 4,
    };

    static constexpr u8 indexBitCycles[256] {
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    };

    enum class AddressMode {
        Null,
        Immediate,
        ImmediateEx,
        Extended,
        IndexedIX,
        IndexedIY,
        Interrupt,
        Refresh,
        Accumulator,
        Flag,
        RegisterB,
        RegisterC,
        RegisterD,
        RegisterE,
        RegisterH,
        RegisterL,
        RegisterBC,
        RegisterDE,
        RegisterHL,
        RegisterSP,
        RegisterAF,
        RegisterIX,
        RegisterIXH,
        RegisterIXL,
        RegisterIY,
        RegisterIYH,
        RegisterIYL,
        RegisterIndirectBC,
        RegisterIndirectDE,
        RegisterIndirectHL,
        RegisterIndirectSP,
    };
    enum class Condition { Null, C, NC, Z, NZ, PO, PE, P, M };

    template<AddressMode mode>
    void setOperand(u16 val)
    {
        switch (mode) {
            case AddressMode::Extended: memory.write8(fetch16(), val); return;
            case AddressMode::IndexedIX: {
                wz = static_cast<u16>(static_cast<s8>(fetch8()) + ix);
                memory.write8(wz, val);
                return;
            } case AddressMode::IndexedIY: {
                wz = static_cast<u16>(static_cast<s8>(fetch8()) + iy);
                memory.write8(wz, val);
                return;
            } case AddressMode::Interrupt: i = val; return;
            case AddressMode::Refresh: r = val; return;
            case AddressMode::Accumulator: a = val; return;
            case AddressMode::RegisterB: regs.b = val; return;
            case AddressMode::RegisterC: regs.c = val; return;
            case AddressMode::RegisterD: regs.d = val; return;
            case AddressMode::RegisterE: regs.e = val; return;
            case AddressMode::RegisterH: regs.h = val; return;
            case AddressMode::RegisterL: regs.l = val; return;
            case AddressMode::RegisterBC: regs.b = val >> 8U; regs.c = val & 0xFF; return;
            case AddressMode::RegisterDE: regs.d = val >> 8U; regs.e = val & 0xFF; return;
            case AddressMode::RegisterHL: regs.h = val >> 8U; regs.l = val & 0xFF; return;
            case AddressMode::RegisterSP: sp = val; return;
            case AddressMode::RegisterAF: {
                a = val >> 8U;
                setf(flags, val & 0xFF);
                return;
            }
            case AddressMode::RegisterIX: ix = val; return;
            case AddressMode::RegisterIXH: ix = (ix & 0x00FFU) | (val << 8U); return;
            case AddressMode::RegisterIXL: ix = (ix & 0xFF00U) | (val & 0xFF); return;
            case AddressMode::RegisterIY: iy = val; return;
            case AddressMode::RegisterIYH: iy = (iy & 0x00FFU) | (val << 8U); return;
            case AddressMode::RegisterIYL: iy = (iy & 0xFF00U) | (val & 0xFF); return;
            case AddressMode::RegisterIndirectBC: memory.write8(regs.b << 8 | regs.c, val); return;
            case AddressMode::RegisterIndirectDE: memory.write8(regs.d << 8 | regs.e, val); return;
            case AddressMode::RegisterIndirectHL: memory.write8(regs.h << 8 | regs.l, val); return;
            case AddressMode::RegisterIndirectSP: memory.write8(sp, val); return;
        }
    }

    template<AddressMode mode>
    u16 getOperand()
    {
        switch (mode) {
            case AddressMode::Immediate: return fetch8();
            case AddressMode::ImmediateEx: return fetch16();
            case AddressMode::Extended: return memory.read8(fetch16());
            case AddressMode::IndexedIX: {
                wz = static_cast<u16>(static_cast<s8>(fetch8()) + ix);
                return memory.read8(wz);
            } case AddressMode::IndexedIY: {
                wz = static_cast<u16>(static_cast<s8>(fetch8()) + iy);
                return memory.read8(wz);
            } case AddressMode::Interrupt: return i;
            case AddressMode::Refresh: return r;
            case AddressMode::Accumulator: return a;
            case AddressMode::RegisterB: return regs.b;
            case AddressMode::RegisterC: return regs.c;
            case AddressMode::RegisterD: return regs.d;
            case AddressMode::RegisterE: return regs.e;
            case AddressMode::RegisterH: return regs.h;
            case AddressMode::RegisterL: return regs.l;
            case AddressMode::RegisterBC: return pair(regs.b, regs.c);
            case AddressMode::RegisterDE: return pair(regs.d, regs.e);
            case AddressMode::RegisterHL: return pair(regs.h, regs.l);
            case AddressMode::RegisterSP: return sp;
            case AddressMode::RegisterAF: return a << 8U | getf();
            case AddressMode::RegisterIX: return ix;
            case AddressMode::RegisterIXH: return ix >> 8U;
            case AddressMode::RegisterIXL: return ix & 0xFF;
            case AddressMode::RegisterIY: return iy;
            case AddressMode::RegisterIYH: return iy >> 8U;
            case AddressMode::RegisterIYL: return iy & 0xFF;
            case AddressMode::RegisterIndirectBC: return memory.read8(pair(regs.b, regs.c));
            case AddressMode::RegisterIndirectDE: return memory.read8(pair(regs.d, regs.e));
            case AddressMode::RegisterIndirectHL: return memory.read8(pair(regs.h, regs.l));
            case AddressMode::RegisterIndirectSP: return memory.read8(sp);
            case AddressMode::Null: return 0;
        }
    }

    template<Condition cond>
    bool getCondition()
    {
        switch (cond) {
            case Condition::C: return flags.cf == cbit;
            case Condition::NC: return flags.cf == 0;
            case Condition::Z: return flags.zf == zbit;
            case Condition::NZ: return flags.zf == 0;
            case Condition::PO: return flags.pf == 0;
            case Condition::PE: return flags.pf == pbit;
            case Condition::P: return flags.sf == 0;
            case Condition::M: return flags.sf == sbit;
            case Condition::Null: return true;
        }
    }

    template<AddressMode src, AddressMode dst>
    void ldWZ()
    {
        if (src == AddressMode::Accumulator and dst == AddressMode::Extended)
            wz = memory.read16(pc) + 1; // MEMPTR = addr + 1
        else if (src == AddressMode::Extended and dst == AddressMode::Accumulator)
            wz = pair(a, (memory.read16(pc) + 1) & 0xFF); // MEMPTR_low = (addr + 1) & #FF,  MEMPTR_hi = A
        else if (src == AddressMode::Accumulator and dst == AddressMode::RegisterIndirectBC)
            wz = pair(regs.b, regs.c) + 1; // MEMPTR = rp + 1
        else if (src == AddressMode::Accumulator and dst == AddressMode::RegisterIndirectDE)
            wz = pair(regs.d, regs.e) + 1; // MEMPTR = rp + 1
        else if (src == AddressMode::RegisterIndirectBC and dst == AddressMode::Accumulator)
            wz = pair(a, (pair(regs.b, regs.c) + 1) & 0xFF); // MEMPTR_low = (rp + 1) & #FF,  MEMPTR_hi = A
        else if (src == AddressMode::RegisterIndirectDE and dst == AddressMode::Accumulator)
            wz = pair(a, (pair(regs.d, regs.e) + 1) & 0xFF); // MEMPTR_low = (rp + 1) & #FF,  MEMPTR_hi = A
        else if (dst == AddressMode::Extended)
            wz = getOperand<src>();
        else if (src == AddressMode::IndexedIX or dst == AddressMode::IndexedIX)
            wz = static_cast<s8>(memory.read8(pc)) + ix;
        else if (src == AddressMode::IndexedIY or dst == AddressMode::IndexedIY)
            wz = static_cast<s8>(memory.read8(pc)) + iy;
    }

    /**
     * \brief Internal function used to execute a single instruction (macro op).
     */
    void tick()
    {
        u8 opcode {fetch8()};
        (this->*instruction[opcode])();
        requested -= cycles[opcode];
    }

    // Helper member functions
    void incrementRefresh() { r = (r & 0x80U) | (r + 1U & ~0x80U); }
    [[nodiscard]] static bool halfcy(const u8 addend, const u8 augend, const u16 sum) { return (addend ^ augend ^ sum) & 0x10U; }
    [[nodiscard]] static bool halfcy(const u16 addend, const u16 augend, const u32 sum) { return (addend ^ augend ^ sum) & 0x1000U; }
    [[nodiscard]] static bool underflow(const u8 addend, const u8 augend, const u16 sum) { return ((addend ^ sum) & (augend ^ sum) & 0x80U) != 0; }
    [[nodiscard]] static bool overflow(const u8 minuend, const u8 subtrahend, const u16 difference) { return ((difference ^ minuend) & (subtrahend ^ minuend) & 0x80U) != 0; }
    [[nodiscard]] u8 fetch8() { return memory.read8(pc++); }
    [[nodiscard]] u16 fetch16() { pc += 2; return memory.read16(pc - 2); };
    [[nodiscard]] static u16 pair(const u8 hi, const u8 lo) { return hi << 8U | lo; }
    [[nodiscard]] static bool parity(const u8 val) { return std::popcount(val) % 2 == 0; }

    u16 top()
    {
        const u16 top {memory.read16(sp)};
        sp += 2;
        return top;
    }

    void pushpc()
    {
        memory.write8(--sp, pc >> 8U);
        memory.write8(--sp, pc & 0xFFU);
    }

    void commonFlags(const u8 val)
    {
        flags.sf = val & sbit;
        flags.zf = val ? 0 : zbit;
        flags.yf = val & ybit;
        flags.xf = val & xbit;
    }

    void logicFlags()
    {
        flags.cf = 0;
        flags.nf = 0;
        flags.pf = parity(a) ? pbit : 0;
        commonFlags(a);
    }

    void rotateFlags(const u8 val)
    {
        flags.pf = parity(val) ? pbit : 0;
        flags.hf = 0;
        flags.nf = 0;
        commonFlags(val);
    }

    void rotateFlagsA(const u8 val)
    {
        flags.hf = 0;
        flags.nf = 0;
        flags.yf = val & ybit;
        flags.xf = val & xbit;
    }

    void blockIOFlags(int n)
    {
        --regs.b;
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) + n);
        flags.nf = nbit;
        flags.zf = regs.b ? zbit : 0;
    }

    void addition(const u8 augend, const u16 sum)
    {
        flags.cf = sum > 0xFFU;
        flags.hf = halfcy(a, augend, sum) ? hbit : 0;
        flags.pf = underflow(a, augend, sum) ? pbit : 0;
        flags.nf = 0;
        a = static_cast<u8>(sum);
        commonFlags(a);
    }

    void subtraction(const u8 subtrahend, const u16 difference)
    {
        flags.hf = halfcy(a, subtrahend, difference) ? hbit : 0;
        flags.pf = overflow(a, subtrahend, difference) ? pbit : 0;
        flags.nf = nbit;
        a = static_cast<u8>(difference);
        commonFlags(a);
    }

    // Load and Exchange Instructions
    template<AddressMode mode1, AddressMode mode2>
    void ld()
    {
        ldWZ<mode1, mode2>(); // must be first line (else pc - 2 required in specific branches in function call)
        if ((mode1 == AddressMode::IndexedIX and mode2 == AddressMode::Immediate) or (mode1 == AddressMode::IndexedIY and mode2 == AddressMode::Immediate)) {
            ++pc;
            memory.write8(wz, fetch8());
        } else
            setOperand<mode1>(getOperand<mode2>());
        flagsWereModified = false;
    }

    template<AddressMode src>
    void ldAddrRp()
    {
        const u16 addr {fetch16()};
        memory.write16(addr, getOperand<src>());
        wz = addr + 1;
        flagsWereModified = false;
    }

    template<AddressMode dst>
    void ldRpAddr()
    {
        const u16 addr {fetch16()};
        setOperand<dst>(memory.read16(addr));
        wz = addr + 1;
        flagsWereModified = false;
    }

    template<AddressMode mode>
    void ldA()
    {
        a = getOperand<mode>();
        flags.hf = 0;
        flags.nf = 0;
        flags.pf = iff2 ? pbit : 0;
        commonFlags(a);
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void push()
    {
        const u16 operand {getOperand<mode>()};
        memory.write8(--sp, operand >> 8U);
        memory.write8(--sp, operand & 0xFFU);
        flagsWereModified = false;
    }

    template<AddressMode mode>
    void pop()
    {
        setOperand<mode>(top());
        flagsWereModified = false;
    }

    void ex1()
    {
        std::swap(a, a2);
        std::swap(flags, flags2);

        flagsWereModified = false;
    }

    template<AddressMode mode>
    void ex2()
    {
        const u16 tmp {getOperand<mode>()};
        setOperand<mode>(pair(memory.read8(sp + 1), memory.read8(sp)));
        memory.write16(sp, tmp);
        wz = getOperand<mode>();
        flagsWereModified = false;
    }

    void ex3()
    {
        std::swap(regs.d, regs.h);
        std::swap(regs.e, regs.l);
        flagsWereModified = false;
    }

    void exx()
    {
        std::swap(regs, regs2);
        flagsWereModified = false;
    }

    // Block Transfer and Search Instructions
    void ldi()
    {
        const u8 tmp {memory.read8(pair(regs.h, regs.l))};
        memory.write8(pair(regs.d, regs.e), tmp);
        setOperand<AddressMode::RegisterBC>(pair(regs.b, regs.c) - 1);
        setOperand<AddressMode::RegisterDE>(pair(regs.d, regs.e) + 1);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) + 1);

        flags.nf = 0;
        flags.hf = 0;
        flags.pf = pair(regs.b, regs.c) ? pbit : 0;

        const u8 n {static_cast<u8>(tmp + a)};
        flags.yf = (n & 0b10) << 4U;
        flags.xf = n & xbit;
        flagsWereModified = true;
    }

    void ldir()
    {
        ldi();
        if (flags.pf) {
            pc -= 2;
            requested -= 5;
            wz = pc + 1;
        }
    }

    void ldd()
    {
        const u8 tmp {memory.read8(pair(regs.h, regs.l))};
        memory.write8(pair(regs.d, regs.e), tmp);
        setOperand<AddressMode::RegisterBC>(pair(regs.b, regs.c) - 1);
        setOperand<AddressMode::RegisterDE>(pair(regs.d, regs.e) - 1);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) - 1);

        flags.nf = 0;
        flags.hf = 0;
        flags.pf = pair(regs.b, regs.c) ? pbit : 0;

        const u8 n {static_cast<u8>(tmp + a)};
        flags.yf = (n & 0b10) << 4U;
        flags.xf = n & xbit;
        flagsWereModified = true;
    }

    void lddr()
    {
        ldd();
        if (flags.pf) {
            pc -= 2;
            requested -= 5;
            wz = pc + 1;
        }
    }

    void cpi()
    {
        const u8 subtrahend {memory.read8(pair(regs.h, regs.l))};
        const u16 difference {static_cast<u16>(a - subtrahend)};
        const u8 tmp {static_cast<u8>(difference)};
        setOperand<AddressMode::RegisterBC>(pair(regs.b, regs.c) - 1);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) + 1);

        flags.sf = tmp & sbit;
        flags.zf = difference ? 0 : zbit;
        flags.hf = halfcy(a, subtrahend, difference) ? hbit : 0;
        flags.pf = pair(regs.b, regs.c) ? pbit : 0;
        flags.nf = nbit;

        const u8 n {static_cast<u8>(difference - (flags.hf >> 4U))};
        flags.yf = (n & 0b10) << 4U;
        flags.xf = n & xbit;
        ++wz;
        flagsWereModified = true;
    }

    void cpir()
    {
        cpi();
        if (flags.pf and !flags.zf) {
            pc -= 2;
            requested -= 5;
            wz = pc + 1;
        } else {
            ++wz;
        }
    }

    void cpd()
    {
        const u8 subtrahend {memory.read8(pair(regs.h, regs.l))};
        const u16 difference {static_cast<u16>(a - subtrahend)};
        const u8 tmp {static_cast<u8>(difference)};
        setOperand<AddressMode::RegisterBC>(pair(regs.b, regs.c) - 1);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) - 1);

        flags.sf = tmp & sbit;
        flags.zf = difference ? 0 : zbit;
        flags.hf = halfcy(a, subtrahend, difference) ? hbit : 0;
        flags.pf = pair(regs.b, regs.c) ? pbit : 0;
        flags.nf = nbit;

        const u8 n {static_cast<u8>(difference - (flags.hf >> 4U))};
        flags.yf = (n & 0b10) << 4U;
        flags.xf = n & xbit;
        --wz;
        flagsWereModified = true;
    }

    void cpdr()
    {
        cpd();
        if (flags.pf and !flags.zf) {
            pc -= 2;
            requested -= 5;
            wz = pc + 1;
        } else {
            ++wz;
        }
    }

    // Arithmetic and Logical Instructions
    template<AddressMode mode>
    void add8()
    {
        const u8 augend {static_cast<u8>(getOperand<mode>())};
        const u16 sum {static_cast<u16>(a + augend)};
        addition(augend, sum);
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void adc8()
    {
        const u8 augend {static_cast<u8>(getOperand<mode>())};
        const u16 sum {static_cast<u16>(a + augend + flags.cf)};
        addition(augend, sum);
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void sub()
    {
        const u8 subtrahend {static_cast<u8>(getOperand<mode>())};
        const u16 difference {static_cast<u16>(a - subtrahend)};
        flags.cf = a < subtrahend;
        subtraction(subtrahend, difference);
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void sbc8()
    {
        const u8 subtrahend {static_cast<u8>(getOperand<mode>())};
        const u16 difference {static_cast<u16>(a - subtrahend - flags.cf)};
        flags.cf = a < static_cast<u16>(subtrahend + flags.cf);
        subtraction(subtrahend, difference);
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void inc8()
    {
        const u8 operand {static_cast<u8>(getOperand<mode>())};
        const u8 res {static_cast<u8>(operand + 1U)};
        flags.hf = (res & 0xFU) != 0 ? 0 : hbit; // only case for half carry is 0b1111 + 0b1 = 0b10000;
        flags.pf = underflow(operand, 1U, res) != 0 ? pbit : 0;
        flags.nf = 0;
        commonFlags(res);
        if (mode == AddressMode::IndexedIX)
            memory.write8(static_cast<s8>(memory.read8(pc - 1)) + ix, res);
        else if (mode == AddressMode::IndexedIY)
            memory.write8(static_cast<s8>(memory.read8(pc - 1)) + iy, res);
        else
            setOperand<mode>(res);
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void dec8()
    {
        const u8 operand {static_cast<u8>(getOperand<mode>())};
        const u8 res {static_cast<u8>(operand - 1U)};
        flags.hf = (res & 0xFU) != 0xF ? 0 : hbit; // only case for half borrow is 0b10000 - 0b1 = 0b1111
        flags.pf = overflow(operand, 1U, res) != 0 ? pbit : 0;
        flags.nf = nbit;
        commonFlags(res);
        if (mode == AddressMode::IndexedIX)
            memory.write8(static_cast<s8>(memory.read8(pc - 1)) + ix, res);
        else if (mode == AddressMode::IndexedIY)
            memory.write8(static_cast<s8>(memory.read8(pc - 1)) + iy, res);
        else
            setOperand<mode>(res);
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void land()
    {
        a &= static_cast<u8>(getOperand<mode>());
        flags.hf = hbit;
        logicFlags();
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void lxor()
    {
        a ^= static_cast<u8>(getOperand<mode>());
        flags.hf = 0;
        logicFlags();
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void lor()
    {
        a |= static_cast<u8>(getOperand<mode>());
        flags.hf = 0;
        logicFlags();
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void cp()
    {
        const u8 subtrahend {static_cast<u8>(getOperand<mode>())};
        const u16 difference {static_cast<u16>(a - subtrahend)};
        flags.cf = a < subtrahend;
        flags.hf = halfcy(a, subtrahend, difference) ? hbit : 0;
        flags.pf = overflow(a, subtrahend, difference) ? pbit : 0;
        flags.nf = nbit;
        flags.yf = subtrahend & ybit;
        flags.xf = subtrahend & xbit;
        flags.zf = difference ? 0 : zbit;
        flags.sf = static_cast<u8>(difference) & sbit;
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void add16()
    {
        const u16 addend {getOperand<mode1>()};
        wz = addend + 1;
        const u16 augend {getOperand<mode2>()};
        const u32 sum {static_cast<u32>(addend + augend)};
        flags.cf = sum > 0xFFFFU;
        flags.hf = halfcy(addend, augend, sum) ? hbit : 0;
        flags.nf = 0;
        flags.yf = static_cast<u16>(sum) & 0x2000 ? ybit : 0;
        flags.xf = static_cast<u16>(sum) & 0x800 ? xbit : 0;
        setOperand<mode1>(static_cast<u16>(sum));
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void adc16()
    {
        const u16 addend {getOperand<mode1>()};
        wz = addend + 1;
        const u16 augend {getOperand<mode2>()};
        const u32 sum {static_cast<u32>(addend + augend + flags.cf)};
        flags.cf = sum > 0xFFFFU;
        flags.hf = halfcy(addend, augend, sum) ? hbit : 0;
        flags.pf = (addend ^ sum) & (augend ^ sum) & 0x8000U ? pbit : 0;
        flags.nf = 0;
        flags.sf = static_cast<u16>(sum) & 0x8000 ? sbit : 0;
        flags.zf = sum ? 0 : zbit;
        flags.yf = static_cast<u16>(sum) & 0x2000 ? ybit : 0;
        flags.xf = static_cast<u16>(sum) & 0x800 ? xbit : 0;
        setOperand<mode1>(static_cast<u16>(sum));
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void sbc16()
    {
        const u16 minuend {getOperand<mode1>()};
        wz = minuend + 1;
        const u16 subtrahend {getOperand<mode2>()};
        const u32 difference {static_cast<u16>(minuend - subtrahend - flags.cf)};
        flags.cf = minuend < static_cast<u32>(subtrahend + flags.cf);
        flags.hf = halfcy(minuend, subtrahend, difference) ? hbit : 0;
        flags.pf = (difference ^ minuend) & (subtrahend ^ minuend) & 0x8000U ? pbit : 0;
        flags.nf = nbit;
        flags.sf = static_cast<u16>(difference) & 0x8000 ? sbit : 0;
        flags.zf = difference ? 0 : zbit;
        flags.yf = static_cast<u16>(difference) & 0x2000 ? ybit : 0;
        flags.xf = static_cast<u16>(difference) & 0x800 ? xbit : 0;
        setOperand<mode1>(static_cast<u16>(difference));
        flagsWereModified = true;
    }

    template<AddressMode mode>
    void inc16()
    {
        setOperand<mode>(getOperand<mode>() + 1);
        flagsWereModified = false;
    }

    template<AddressMode mode>
    void dec16()
    {
        setOperand<mode>(getOperand<mode>() - 1);
        flagsWereModified = false;
    }

    void cpl()
    {
        a = ~a;
        flags.nf = nbit;
        flags.hf = hbit;
        flags.yf = a & ybit;
        flags.xf = a & xbit;
        flagsWereModified = true;
    }

    void neg()
    {
        const u8 res {static_cast<u8>(0 - a)};
        flags.cf = a != 0 ? cbit : 0;
        flags.pf = a == 0x80U ? pbit : 0;
        flags.hf = halfcy(a, 0, res) ? hbit : 0;
        flags.nf = nbit;
        a = res;
        commonFlags(a);
        flagsWereModified = true;
    }

    void ccf()
    { // ccf sets xf and yf differently based on whether the last instruction modified the flag register
        flags.hf = flags.cf ? hbit : 0;
        flags.cf = flags.cf ? 0 : cbit;
        flags.nf = 0;
        flags.yf = flagsWereModified ? ybit & a : flags.yf | a & ybit;
        flags.xf = flagsWereModified ? xbit & a : flags.xf | a & xbit;
        flagsWereModified = true;
    }

    void scf()
    { // scf sets xf and yf differently based on whether the last instruction modified the flag register
        flags.cf = cbit;
        flags.nf = 0;
        flags.hf = 0;
        flags.yf = flagsWereModified ? ybit & a : flags.yf | a & ybit;
        flags.xf = flagsWereModified ? xbit & a : flags.xf | a & xbit;
        flagsWereModified = true;
    }

    void daa()
    {
        u8 correction = 0;

        if ((a & 0x0F) > 0x09 or flags.hf) {
            correction |= 0x06;
        }

        if (a > 0x99 or flags.cf) {
            correction |= 0x60;
            flags.cf = cbit;
        } else {
            flags.cf = 0;
        }

        if (flags.nf) {
            flags.hf = flags.hf and (a & 0x0F) < 0x06 ? hbit : 0;
            a -= correction;
        } else {
            flags.hf = (a & 0x0F) > 0x09 ? hbit : 0;
            a += correction;
        }

        flags.pf = parity(a) ? pbit : 0;
        commonFlags(a);
        flagsWereModified = true;
    }

    // Rotate and Shift Instructions
    void rlca()
    {
        flags.cf = a & 0x80U ? cbit : 0;
        a = (a << 1U) | flags.cf;
        rotateFlagsA(a);
        flagsWereModified = true;
    }

    void rla()
    {
        const u8 tmp {flags.cf};
        flags.cf = a & 0x80U ? cbit : 0;
        a = (a << 1U) | tmp;
        rotateFlagsA(a);
        flagsWereModified = true;
    }

    void rrca()
    {
        flags.cf = a & 0x01U ? cbit : 0;
        a = (a >> 1U) | (flags.cf << 7U);
        rotateFlagsA(a);
        flagsWereModified = true;
    }

    void rra()
    {
        const u8 tmp {flags.cf};
        flags.cf = a & 0x01U ? cbit : 0;
        a = (a >> 1U) | (tmp << 7U);
        rotateFlagsA(a);
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void storeBitResult(const u8 val)
    {
        // After getOperand() call, the pc is pointing at the opcode because the displacement byte is before the opcode.
        // To write to the correct spot, have to decrement the pc. Then, in order to point to the next opcode, we have to
        // add 2 to the pc.
        if (mode1 == AddressMode::IndexedIX) {
            memory.write8(static_cast<s8>(memory.read8(--pc)) + ix, val);
            pc += 2;
        } else if (mode1 == AddressMode::IndexedIY) {
            memory.write8(static_cast<s8>(memory.read8(--pc)) + iy, val);
            pc += 2;
        }

        if (mode2 != AddressMode::IndexedIX and mode2 != AddressMode::IndexedIY)
            setOperand<mode2>(val);
    }

    template<AddressMode mode1, AddressMode mode2>
    void rlc()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80 ? cbit : 0;
        val = (val << 1U) | flags.cf;
        rotateFlags(val);
        storeBitResult<mode1, mode2>(val);
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void rl()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        const u8 tmp {flags.cf};
        flags.cf = val & 0x80 ? cbit : 0;
        val = (val << 1U) | tmp;
        rotateFlags(val);
        storeBitResult<mode1, mode2>(val);
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void rrc()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & cbit;
        val = (val >> 1U) | (flags.cf << 7U);
        rotateFlags(val);
        storeBitResult<mode1, mode2>(val);
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void rr()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        const u8 tmp {flags.cf};
        flags.cf = val & cbit;
        val = (val >> 1U) | (tmp << 7U);
        rotateFlags(val);
        storeBitResult<mode1, mode2>(val);
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void sla()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80 ? cbit : 0;
        val <<= 1U;
        rotateFlags(val);
        storeBitResult<mode1, mode2>(val);
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void sll()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80 ? cbit : 0;
        val = val << 1U | 1U;
        rotateFlags(val);
        storeBitResult<mode1, mode2>(val);
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void sra()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & cbit;
        const u8 tmp {static_cast<u8>(val & 0x80U)};
        val = (val >> 1U) | tmp;
        rotateFlags(val);
        storeBitResult<mode1, mode2>(val);
        flagsWereModified = true;
    }

    template<AddressMode mode1, AddressMode mode2>
    void srl()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & cbit;
        val >>= 1U;
        rotateFlags(val);
        storeBitResult<mode1, mode2>(val);
        flagsWereModified = true;
    }

    void rld()
    {
        const u8 byte {memory.read8(pair(regs.h, regs.l))};
        memory.write8(pair(regs.h, regs.l), (byte << 4U) | (a & 0x0FU));
        a = (a & 0xF0U) | (byte >> 4U);
        rotateFlags(a);
        wz = pair(regs.h, regs.l) + 1;
        flagsWereModified = true;
    }

    void rrd()
    {
        const u8 byte {memory.read8(pair(regs.h, regs.l))};
        memory.write8(pair(regs.h, regs.l), (a << 4U) | (byte >> 4U));
        a = (a & 0xF0U) | (byte & 0x0FU);
        rotateFlags(a);
        wz = pair(regs.h, regs.l) + 1;
        flagsWereModified = true;
    }

    // Bit Manipulation Instructions
    template<u8 mask, AddressMode mode>
    void bit()
    {
        const u8 operand {static_cast<u8>(getOperand<mode>())};
        const u8 res {static_cast<u8>(operand & mask)};
        flags.hf = hbit;
        flags.nf = 0;
        flags.sf = res & sbit;
        flags.zf = res ? 0 : zbit;
        flags.pf = flags.zf ? pbit : 0;
        if (mode == AddressMode::IndexedIX or mode == AddressMode::IndexedIY) {
            ++pc;
            flags.yf = wz & 0x2000 ? ybit : 0;
            flags.xf = wz & 0x800 ? xbit : 0;
        } else {
            flags.yf = operand & ybit;
            flags.xf = operand & xbit;
        }
        flagsWereModified = true;
    }

    template<u8 mask>
    void bitHL()
    { // bit n, (hl) sets xf and yf based on value in the z80's internal wz (or memptr) register
        const u16 hl {pair(regs.h, regs.l)};
        const u8 operand {memory.read8(hl)};
        const u8 res {static_cast<u8>(operand & mask)};
        flags.hf = hbit;
        flags.nf = 0;
        flags.sf = res & sbit;
        flags.zf = res ? 0 : zbit;
        flags.pf = flags.zf ? pbit : 0;
        flags.yf = wz & 0x2000 ? ybit : 0;
        flags.xf = wz & 0x800 ? xbit : 0;
        flagsWereModified = true;
    }

    template<u8 mask, AddressMode mode1, AddressMode mode2>
    void res()
    {
        storeBitResult<mode1, mode2>(getOperand<mode1>() & mask);
        flagsWereModified = false;
    }

    template<u8 mask, AddressMode mode1, AddressMode mode2>
    void set()
    {
        storeBitResult<mode1, mode2>(getOperand<mode1>() | mask);
        flagsWereModified = false;
    }

    // Jump, Call, and Return Instructions
    template<Condition cond>
    void jr()
    {
        const s8 d {static_cast<s8>(fetch8())};
        if (getCondition<cond>()) {
            pc += d;

            if (cond != Condition::Null)
                requested -= 5;
            wz = pc;
        }
        flagsWereModified = false;
    }

    template<Condition cond, AddressMode mode>
    void jp()
    {
        const u16 addr {getOperand<mode>()};
        if (getCondition<cond>())
            pc = addr;
        if (mode != AddressMode::RegisterHL and mode != AddressMode::RegisterIX and mode != AddressMode::RegisterIY)
            wz = addr;
        flagsWereModified = false;
    }

    template<Condition cond, AddressMode mode>
    void call()
    {
        const u16 addr {getOperand<mode>()};
        if (getCondition<cond>()) {
            pushpc();
            pc = addr;

            if (cond != Condition::Null)
                requested -= 7;
        }
        wz = addr;
        flagsWereModified = false;
    }

    void djnz()
    {
        const s8 val {static_cast<s8>(fetch8())};
        if (--regs.b != 0 ) {
            pc += val;
            requested -= 5;
        }
        if (regs.b != 0)
            wz = pc;
        flagsWereModified = false;
    }

    template<Condition cond>
    void ret()
    {
        if (getCondition<cond>()) {
            pc = top();

            if (cond != Condition::Null)
                requested -= 6;
            wz = pc;
        }
        flagsWereModified = false;
    }

    void retn()
    {
        pc = wz = top();
        iff1 = iff2;
        flagsWereModified = false;
    }

    template<u8 p>
    void rst()
    {
        pushpc();
        pc = wz = p;
        flagsWereModified = false;
    }

    // Input/Output Instructions
    void in()
    {
        const u8 port {fetch8()};
        wz = pair(a, port) + 1;
        a = memory.input(this, port);
        flagsWereModified = false;
    }

    template<AddressMode mode>
    void in()
    {
        const u8 c {memory.input(this, regs.c)};
        if (mode != AddressMode::Flag)
            setOperand<mode>(c);
        rotateFlags(c);
        wz = pair(regs.b, regs.c) + 1;
        flagsWereModified = true;
    }

    void ini()
    {
        const u8 tmp {memory.input(this, regs.c)};
        memory.write8(pair(regs.h, regs.l), tmp);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) + 1);

        --regs.b;
        commonFlags(regs.b);
        flags.nf = (tmp & sbit) >> 5U;

        const u16 tmp2 {static_cast<u16>(tmp + ((regs.c + 1) & 0xFF))};
        flags.pf = parity((tmp2 & 7U) ^ regs.b) ? pbit : 0;
        flags.hf = tmp2 > 0xFFU ? hbit : 0;
        flags.cf = tmp2 > 0xFFU ? cbit : 0;
        wz = pair(regs.b + 1, regs.c) + 1;
        flagsWereModified = true;
    }

    void inir()
    {
        ini();
        if (!flags.zf) {
            pc -= 2;
            requested -= 5;
            wz = pc + 1;
        }
    }

    void ind()
    {
        const u8 tmp {memory.input(this, regs.c)};
        memory.write8(pair(regs.h, regs.l), tmp);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) - 1);

        --regs.b;
        commonFlags(regs.b);
        flags.nf = (tmp & sbit) >> 5U;

        const u16 tmp2 {static_cast<u16>(tmp + ((regs.c + 1) & 0xFF))};
        flags.pf = parity((tmp2 & 7U) ^ regs.b) ? pbit : 0;
        flags.hf = tmp2 > 0xFFU ? hbit : 0;
        flags.cf = tmp2 > 0xFFU ? cbit : 0;
        wz = pair(regs.b + 1, regs.c) - 1;
        flagsWereModified = true;
    }

    void indr()
    {
        ind();
        if (!flags.zf) {
            pc -= 2;
            requested -= 5;
            wz = pc + 1;
        }
    }

    void out()
    {
        const u8 port {fetch8()};
        memory.output(this, port, a);
        wz = pair(a, (port + 1) & 0xFF);
    }

    template<AddressMode mode>
    void out()
    {
        memory.output(this, regs.c, getOperand<mode>());
        if (mode != AddressMode::Null) wz = pair(regs.b, regs.c) + 1;
    }

    void outi()
    {
        const u8 tmp {memory.read8(pair(regs.h, regs.l))};
        memory.output(this, regs.c, tmp);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) + 1);

        --regs.b;
        commonFlags(regs.b);
        flags.nf = (tmp & sbit) >> 5U;

        const u16 tmp2 {static_cast<u16>(tmp + regs.l)};
        flags.pf = parity((tmp2 & 7U) ^ regs.b) ? pbit : 0;
        flags.hf = tmp2 > 0xFFU ? hbit : 0;
        flags.cf = tmp2 > 0xFFU ? cbit : 0;
        wz = pair(regs.b, regs.c) + 1;
        flagsWereModified = true;
    }

    void otir()
    {
        outi();
        if (!flags.zf) {
            pc -= 2;
            requested -= 5;
            wz = pc + 1;
        }
    }

    void outd()
    {
        const u8 tmp {memory.read8(pair(regs.h, regs.l))};
        memory.output(this, regs.c, tmp);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) - 1);

        --regs.b;
        commonFlags(regs.b);
        flags.nf = (tmp & sbit) >> 5U;

        const u16 tmp2 {static_cast<u16>(tmp + regs.l)};
        flags.pf = parity((tmp2 & 7U) ^ regs.b) ? pbit : 0;
        flags.hf = tmp2 > 0xFFU ? hbit : 0;
        flags.cf = tmp2 > 0xFFU ? cbit : 0;
        wz = pair(regs.b, regs.c) - 1;
        flagsWereModified = true;
    }

    void otdr()
    {
        outd();
        if (!flags.zf) {
            pc -= 2;
            requested -= 5;
            wz = pc + 1;
        }
    }

    // CPU Control Group Instructions
    void nop() { flagsWereModified = false; }
    void halt()
    {
        halted = true;
        pc -= 1;
        flagsWereModified = false;
    }
    void di()
    {
        iff1 = iff2 = false;
        flagsWereModified = false;
    }
    void ei()
    {
        iff1 = iff2 = true;
        flagsWereModified = false;
    }
    template<u8 n>
    void im()
    {
        imode = n;
        flagsWereModified = false;
    }

    // Prefix Instructions
    void preMisc()
    {
        const u8 opcode {fetch8()};
        incrementRefresh();

        (this->*miscInstruction[opcode])();
        requested -= miscCycles[opcode];
    }

    void preBit()
    {
        const u8 opcode {fetch8()};
        incrementRefresh();

        (this->*bitInstruction[opcode])();
        requested -= bitCycles[opcode];
    }

    template<AddressMode mode, AddressMode modeH, AddressMode modeL, AddressMode modeD>
    void preIndex()
    {
        const u8 opcode {fetch8()};
        incrementRefresh();

        (this->*indexInstruction<mode, modeH, modeL, modeD>[opcode])();
        requested -= indexCycles[opcode];
    }

    template<AddressMode mode>
    void preIndexBit()
    {
        ++pc; // skip over displacement byte
        const u8 opcode {memory.read8(pc--)}; // subtract one so next instruction will get the displacement byte

        (this->*indexBitInstruction<mode>[opcode])();
        requested -= indexBitCycles[opcode];
    }

    // Primary Instruction Table
    static constexpr std::array<void (z80::*)(), 256> instruction {
        &z80::nop, // $00: nop
        &z80::ld<AddressMode::RegisterBC, AddressMode::ImmediateEx>, // $01: ld bc, nn
        &z80::ld<AddressMode::RegisterIndirectBC, AddressMode::Accumulator>, // $02: ld (bc), a
        &z80::inc16<AddressMode::RegisterBC>, // $03: inc bc
        &z80::inc8<AddressMode::RegisterB>, // $04: inc b
        &z80::dec8<AddressMode::RegisterB>, // $05: dec b
        &z80::ld<AddressMode::RegisterB, AddressMode::Immediate>, // $06: ld b, n
        &z80::rlca, // $07: rlca,
        &z80::ex1, // $08: ex af, af'
        &z80::add16<AddressMode::RegisterHL, AddressMode::RegisterBC>, // $09: add hl, bc
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterIndirectBC>, // $0A: ld a, (bc)
        &z80::dec16<AddressMode::RegisterBC>, // $0B: dec bc
        &z80::inc8<AddressMode::RegisterC>, // $0C: inc c
        &z80::dec8<AddressMode::RegisterC>, // $0D: dec c
        &z80::ld<AddressMode::RegisterC, AddressMode::Immediate>, // $0E: ld c, n
        &z80::rrca, // $0F: rrca
        &z80::djnz, // $10: djnz d
        &z80::ld<AddressMode::RegisterDE, AddressMode::ImmediateEx>, // $11: ld de, nn
        &z80::ld<AddressMode::RegisterIndirectDE, AddressMode::Accumulator>, // $12: ld (de), a
        &z80::inc16<AddressMode::RegisterDE>, // $13: inc de
        &z80::inc8<AddressMode::RegisterD>, // $14: inc d
        &z80::dec8<AddressMode::RegisterD>, // $15: dec d
        &z80::ld<AddressMode::RegisterD, AddressMode::Immediate>, // $16: ld d, n
        &z80::rla, // $17: rla
        &z80::jr<Condition::Null>, // $18: jr d
        &z80::add16<AddressMode::RegisterHL, AddressMode::RegisterDE>, // $19: add hl, de
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterIndirectDE>, // $1A: ld a, (de)
        &z80::dec16<AddressMode::RegisterDE>, // $1B: dec de
        &z80::inc8<AddressMode::RegisterE>, // $1C: inc e
        &z80::dec8<AddressMode::RegisterE>, // $1D: dec e
        &z80::ld<AddressMode::RegisterE, AddressMode::Immediate>, // $1E: ld e, n
        &z80::rra, // $1F: rra
        &z80::jr<Condition::NZ>, // $20: jr nz, d
        &z80::ld<AddressMode::RegisterHL, AddressMode::ImmediateEx>, // $21: ld hl, nn
        &z80::ldAddrRp<AddressMode::RegisterHL>, // $22: ld (nn), hl
        &z80::inc16<AddressMode::RegisterHL>, // $23: inc hl
        &z80::inc8<AddressMode::RegisterH>, // $24: inc h
        &z80::dec8<AddressMode::RegisterH>, // $25: dec h
        &z80::ld<AddressMode::RegisterH, AddressMode::Immediate>, // $26: ld h, n
        &z80::daa, // $27: daa
        &z80::jr<Condition::Z>, // $28: jr z, d
        &z80::add16<AddressMode::RegisterHL, AddressMode::RegisterHL>, // $29: add hl, hl
        &z80::ldRpAddr<AddressMode::RegisterHL>, // $2A: ld hl, (nn)
        &z80::dec16<AddressMode::RegisterHL>, // $2B: dec hl
        &z80::inc8<AddressMode::RegisterL>, // $2C: inc l
        &z80::dec8<AddressMode::RegisterL>, // $2D: dec l
        &z80::ld<AddressMode::RegisterL, AddressMode::Immediate>, // $2E: ld l, n
        &z80::cpl, // $2F: cpl
        &z80::jr<Condition::NC>, // $30: jr nc, d
        &z80::ld<AddressMode::RegisterSP, AddressMode::ImmediateEx>, // $31 ld sp, nn
        &z80::ld<AddressMode::Extended, AddressMode::Accumulator>, // $32: ld (nn), a
        &z80::inc16<AddressMode::RegisterSP>, // $33: inc sp
        &z80::inc8<AddressMode::RegisterIndirectHL>, // $34: inc (hl)
        &z80::dec8<AddressMode::RegisterIndirectHL>, // $35: dec (hl)
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::Immediate>, // $36: ld (hl), n
        &z80::scf, // $37: scf
        &z80::jr<Condition::C>, // $38: jr c, d
        &z80::add16<AddressMode::RegisterHL, AddressMode::RegisterSP>, // $39: add hl, sp
        &z80::ld<AddressMode::Accumulator, AddressMode::Extended>, // $3A: ld a, (nn)
        &z80::dec16<AddressMode::RegisterSP>, // $3B: dec sp
        &z80::inc8<AddressMode::Accumulator>, // $3C: inc a
        &z80::dec8<AddressMode::Accumulator>, // $3D: dec a
        &z80::ld<AddressMode::Accumulator, AddressMode::Immediate>, // $3E: ld a, n
        &z80::ccf, // $3F: ccf
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterB>, // $40: ld b, b
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterC>, // $41: ld b, c
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterD>, // $42: ld b, d
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterE>, // $43: ld b, e
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterH>, // $44: ld b, h
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterL>, // $45: ld b, l
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterIndirectHL>, // $46: ld b, (hl)
        &z80::ld<AddressMode::RegisterB, AddressMode::Accumulator>, // $47: ld c, a
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterB>, // $48: ld c, b
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterC>, // $49: ld c, c
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterD>, // $4A: ld c, d
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterE>, // $4B: ld c, e
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterH>, // $4C: ld c, h
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterL>, // $4D: ld c, l
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterIndirectHL>, // 4E: $ld c, (hl)
        &z80::ld<AddressMode::RegisterC, AddressMode::Accumulator>, // $4F: ld c, a
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterB>, // $50: ld d, b
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterC>, // $51: ld d, c
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterD>, // $52: ld d, d
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterE>, // $53: ld d, e
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterH>, // $54: ld d, h
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterL>, // $55: ld d, l
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterIndirectHL>, // $56: ld d, (hl)
        &z80::ld<AddressMode::RegisterD, AddressMode::Accumulator>, // $57: ld d, a
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterB>, // $58: ld e, b
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterC>, // $59: ld e, c
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterD>, // $5A: ld e, d
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterE>, // $5B: ld e, e
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterH>, // $5C: ld e, h
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterL>, // $5D: ld e, l
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterIndirectHL>, // $5E: ld e, (hl)
        &z80::ld<AddressMode::RegisterE, AddressMode::Accumulator>, // $5F: ld e, a
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterB>, // $60: ld h, b
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterC>, // $61: ld h, c
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterD>, // $62: ld h, d
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterE>, // $63: ld h, e
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterH>, // $64: ld h, h
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterL>, // $65: ld h, l
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterIndirectHL>, // $66: ld h, (hl)
        &z80::ld<AddressMode::RegisterH, AddressMode::Accumulator>, // $67: ld h, a
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterB>, // $68: ld l, b
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterC>, // $69: ld l, c
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterD>, // $6A: ld l, d
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterE>, // $6B: ld l, e
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterH>, // $6C: ld l, h
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterL>, // $6D: ld l, l
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterIndirectHL>, // $6E: ld l, (hl)
        &z80::ld<AddressMode::RegisterL, AddressMode::Accumulator>, // $6F: ld l, a
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterB>, // $70: ld (hl), b
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterC>, // $71: ld (hl), c
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterD>, // $72: ld (hl), d
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterE>, // $73: ld (hl), e
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterH>, // $74: ld (hl), h
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterL>, // $75: ld (hl), l
        &z80::halt, // $76: halt
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::Accumulator>, // $77: ld (hl), a
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterB>, // $78: ld a, b
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterC>, // $79: ld a, c
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterD>, // $7A: ld a, d
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterE>, // $7B: ld a, e
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterH>, // $7C: ld a, h
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterL>, // $7D: ld a, l
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterIndirectHL>, // $7E: ld a, (hl)
        &z80::ld<AddressMode::Accumulator, AddressMode::Accumulator>, // $7F: ld a, a
        &z80::add8<AddressMode::RegisterB>, // $80: add a, b
        &z80::add8<AddressMode::RegisterC>, // $81: add a, c
        &z80::add8<AddressMode::RegisterD>, // $82: add a, d
        &z80::add8<AddressMode::RegisterE>, // $83: add a, e
        &z80::add8<AddressMode::RegisterH>, // $84: add a, h
        &z80::add8<AddressMode::RegisterL>, // $85: add a, l
        &z80::add8<AddressMode::RegisterIndirectHL>, // $86: add a, (hl)
        &z80::add8<AddressMode::Accumulator>, // $87: add a, a
        &z80::adc8<AddressMode::RegisterB>, // $88: adc a, b
        &z80::adc8<AddressMode::RegisterC>, // $89: adc a, c
        &z80::adc8<AddressMode::RegisterD>, // $8A: adc a, d
        &z80::adc8<AddressMode::RegisterE>, // $8B: adc a, e
        &z80::adc8<AddressMode::RegisterH>, // $8C: adc a, h
        &z80::adc8<AddressMode::RegisterL>, // $8D: adc a, l
        &z80::adc8<AddressMode::RegisterIndirectHL>, // $8E: adc a, (hl)
        &z80::adc8<AddressMode::Accumulator>, // $8F: adc a, a
        &z80::sub<AddressMode::RegisterB>, // $90: sub b
        &z80::sub<AddressMode::RegisterC>, // $91: sub c
        &z80::sub<AddressMode::RegisterD>, // $92: sub d
        &z80::sub<AddressMode::RegisterE>, // $93: sub e
        &z80::sub<AddressMode::RegisterH>, // $94: sub h
        &z80::sub<AddressMode::RegisterL>, // $95: sub l
        &z80::sub<AddressMode::RegisterIndirectHL>, // $96: sub (hl)
        &z80::sub<AddressMode::Accumulator>, // $97: sub a
        &z80::sbc8<AddressMode::RegisterB>, // $98: sbc a, b
        &z80::sbc8<AddressMode::RegisterC>, // $99: sbc a, c
        &z80::sbc8<AddressMode::RegisterD>, // $9A: sbc a, d
        &z80::sbc8<AddressMode::RegisterE>, // $9B: sbc a, e
        &z80::sbc8<AddressMode::RegisterH>, // $9C: sbc a, h
        &z80::sbc8<AddressMode::RegisterL>, // $9D: sbc a, l
        &z80::sbc8<AddressMode::RegisterIndirectHL>, // $9E: sbc a, (hl)
        &z80::sbc8<AddressMode::Accumulator>, // $9F: sbc a, a
        &z80::land<AddressMode::RegisterB>, // $A0: and b
        &z80::land<AddressMode::RegisterC>, // $A1: and c
        &z80::land<AddressMode::RegisterD>, // $A2: and d
        &z80::land<AddressMode::RegisterE>, // $A3: and e
        &z80::land<AddressMode::RegisterH>, // $A4: and h
        &z80::land<AddressMode::RegisterL>, // $A5: and l
        &z80::land<AddressMode::RegisterIndirectHL>, // $A6: and (hl)
        &z80::land<AddressMode::Accumulator>, // $A7: and a
        &z80::lxor<AddressMode::RegisterB>, // $A8: xor b
        &z80::lxor<AddressMode::RegisterC>, // $A9: xor c
        &z80::lxor<AddressMode::RegisterD>, // $AA: xor d
        &z80::lxor<AddressMode::RegisterE>, // $AB: xor e
        &z80::lxor<AddressMode::RegisterH>, // $AC: xor h
        &z80::lxor<AddressMode::RegisterL>, // $AD: xor l
        &z80::lxor<AddressMode::RegisterIndirectHL>, // $AE: xor a, (hl)
        &z80::lxor<AddressMode::Accumulator>, // $AF: xor a
        &z80::lor<AddressMode::RegisterB>, // $B0: or b
        &z80::lor<AddressMode::RegisterC>, // $B1: or c
        &z80::lor<AddressMode::RegisterD>, // $B2: or d
        &z80::lor<AddressMode::RegisterE>, // $B3: or e
        &z80::lor<AddressMode::RegisterH>, // $B4: or h
        &z80::lor<AddressMode::RegisterL>, // $B5: or l
        &z80::lor<AddressMode::RegisterIndirectHL>, // $B6: or (hl)
        &z80::lor<AddressMode::Accumulator>, // $B7: or a
        &z80::cp<AddressMode::RegisterB>, // $B8: cp b
        &z80::cp<AddressMode::RegisterC>, // $B9: cp c
        &z80::cp<AddressMode::RegisterD>, // $BA: cp d
        &z80::cp<AddressMode::RegisterE>, // $BB: cp e
        &z80::cp<AddressMode::RegisterH>, // $BC: cp h
        &z80::cp<AddressMode::RegisterL>, // $BD: cp l
        &z80::cp<AddressMode::RegisterIndirectHL>, // $BE: cp (hl)
        &z80::cp<AddressMode::Accumulator>, // $BF: cp a
        &z80::ret<Condition::NZ>, // $C0: ret nz
        &z80::pop<AddressMode::RegisterBC>, // $C1: pop bc
        &z80::jp<Condition::NZ, AddressMode::ImmediateEx>, // $C2: jp nz, nn
        &z80::jp<Condition::Null, AddressMode::ImmediateEx>, // $C3: jp nn
        &z80::call<Condition::NZ, AddressMode::ImmediateEx>, // $C4: call nz, nn
        &z80::push<AddressMode::RegisterBC>, // $C5: push bc
        &z80::add8<AddressMode::Immediate>, // $C6: add a, n
        &z80::rst<0x00U>, // $C7: rst 0
        &z80::ret<Condition::Z>, // $C8: ret z
        &z80::ret<Condition::Null>, // $C9: ret
        &z80::jp<Condition::Z, AddressMode::ImmediateEx>, // $CA: jp z, nn
        &z80::preBit, // $CB: bit
        &z80::call<Condition::Z, AddressMode::ImmediateEx>, // $CC: call z, nn
        &z80::call<Condition::Null, AddressMode::ImmediateEx>, // $CD: call nn
        &z80::adc8<AddressMode::Immediate>, // $CE: adc a, n
        &z80::rst<0x08U>, // $CF: rst 8
        &z80::ret<Condition::NC>, // $D0: ret nc
        &z80::pop<AddressMode::RegisterDE>, // $D1: pop de
        &z80::jp<Condition::NC, AddressMode::ImmediateEx>, // $D2: jp nc, nn
        &z80::out, // $D3: out (n), a
        &z80::call<Condition::NC, AddressMode::ImmediateEx>, // $D4: call nc, nn
        &z80::push<AddressMode::RegisterDE>, // $D5: push de
        &z80::sub<AddressMode::Immediate>, // $D6: sub n
        &z80::rst<0x10U>, // $D7: rst 16
        &z80::ret<Condition::C>, // $D8: ret c
        &z80::exx, // $D9: exx
        &z80::jp<Condition::C, AddressMode::ImmediateEx>, // $DA: jp c, nn
        &z80::in, // $DB: in a, (n)
        &z80::call<Condition::C, AddressMode::ImmediateEx>, // $DC: call c, nn
        &z80::preIndex<AddressMode::RegisterIX, AddressMode::RegisterIXH, AddressMode::RegisterIXL, AddressMode::IndexedIX>, // $DD: IX
        &z80::sbc8<AddressMode::Immediate>, // $DE: sbc a, n
        &z80::rst<0x18U>, // $DF: rst 24
        &z80::ret<Condition::PO>, // $E0: ret po
        &z80::pop<AddressMode::RegisterHL>, // $E1: pop hl
        &z80::jp<Condition::PO, AddressMode::ImmediateEx>, // $E2: jp po, nn
        &z80::ex2<AddressMode::RegisterHL>, // $E3: ex (sp), hl
        &z80::call<Condition::PO, AddressMode::ImmediateEx>, // $E4: call po, nn
        &z80::push<AddressMode::RegisterHL>, // $E5: push hl
        &z80::land<AddressMode::Immediate>, // $E6: add n
        &z80::rst<0x20U>, // $E7: rst 32
        &z80::ret<Condition::PE>, // $E8: ret pe
        &z80::jp<Condition::Null, AddressMode::RegisterHL>, // $E9: jp (hl)
        &z80::jp<Condition::PE, AddressMode::ImmediateEx>, // $EA: jp pe, nn
        &z80::ex3, // $EB: ex de, hl
        &z80::call<Condition::PE, AddressMode::ImmediateEx>, // $EC: call pe, nn
        &z80::preMisc, // $ED: Misc.
        &z80::lxor<AddressMode::Immediate>, // $EE: xor n
        &z80::rst<0x28U>, // $EF: rst 40
        &z80::ret<Condition::P>, // $F0: ret p
        &z80::pop<AddressMode::RegisterAF>, // $F1: pop af
        &z80::jp<Condition::P, AddressMode::ImmediateEx>, // $F2: jp p, nn
        &z80::di, // $F3: di
        &z80::call<Condition::P, AddressMode::ImmediateEx>, // $F4: call p, nn
        &z80::push<AddressMode::RegisterAF>, // $F5: push af
        &z80::lor<AddressMode::Immediate>, // $F6: or n
        &z80::rst<0x30U>, // $F7: rst 48
        &z80::ret<Condition::M>, // $F8: ret m
        &z80::ld<AddressMode::RegisterSP, AddressMode::RegisterHL>, // $F9: ld sp, hl
        &z80::jp<Condition::M, AddressMode::ImmediateEx>, // $FA: jp m, nn
        &z80::ei, // $FB: ei
        &z80::call<Condition::M, AddressMode::ImmediateEx>, // $FC: call m, nn
        &z80::preIndex<AddressMode::RegisterIY, AddressMode::RegisterIYH, AddressMode::RegisterIYL, AddressMode::IndexedIY>, // $FD: IY
        &z80::cp<AddressMode::Immediate>, // $FE: cp n
        &z80::rst<0x38U>,// $FF: rst 56
    };

    // Misc. Instruction ($ED) Table
    static constexpr std::array<void (z80::*)(), 256> miscInstruction {
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::in<AddressMode::RegisterB>, // $40: in b, (c)
        &z80::out<AddressMode::RegisterB>, // $41: out (c), b
        &z80::sbc16<AddressMode::RegisterHL, AddressMode::RegisterBC>, // $42: sbc, hl, bc
        &z80::ldAddrRp<AddressMode::RegisterBC>, // $43: ld (nn), bc
        &z80::neg, // $44: neg
        &z80::retn, // $45: retn
        &z80::im<0U>, // $46: im 0
        &z80::ld<AddressMode::Interrupt, AddressMode::Accumulator>, // $47: ld i, a
        &z80::in<AddressMode::RegisterC>, // $48: in c, (c)
        &z80::out<AddressMode::RegisterC>, // $49: out (c), c
        &z80::adc16<AddressMode::RegisterHL, AddressMode::RegisterBC>, // $4A: adc hl, bc
        &z80::ldRpAddr<AddressMode::RegisterBC>, // $4B: ld bc, (nn),
        &z80::neg, // $4C: neg
        &z80::retn, // $4D: reti
        &z80::nop,
        &z80::ld<AddressMode::Refresh, AddressMode::Accumulator>, // $4F: ld r, a
        &z80::in<AddressMode::RegisterD>, // $50: in d, (c)
        &z80::out<AddressMode::RegisterD>, // $51: out (c), d
        &z80::sbc16<AddressMode::RegisterHL, AddressMode::RegisterDE>, // $52: sbc hl, de
        &z80::ldAddrRp<AddressMode::RegisterDE>, // $53: ld (nn), de
        &z80::neg, // $54: neg
        &z80::retn, // $55: retn
        &z80::im<1U>, // $56: im 1
        &z80::ldA<AddressMode::Interrupt>, // $57: ld a, i
        &z80::in<AddressMode::RegisterE>, // $58: in e, (c)
        &z80::out<AddressMode::RegisterE>, // $59: out (c), e
        &z80::adc16<AddressMode::RegisterHL, AddressMode::RegisterDE>, // $5A: adc hl, de
        &z80::ldRpAddr<AddressMode::RegisterDE>, // $5B: ld de, (nn)
        &z80::neg, // $5C: neg
        &z80::retn, // $5D: retn
        &z80::im<2U>, // // $5E: im 2
        &z80::ldA<AddressMode::Refresh>, // $5F: ld a, r
        &z80::in<AddressMode::RegisterH>, // $60: in h, (c)
        &z80::out<AddressMode::RegisterH>, // $61: out (c), h
        &z80::sbc16<AddressMode::RegisterHL, AddressMode::RegisterHL>, // $62: sbc hl, hl
        &z80::ldAddrRp<AddressMode::RegisterHL>, // $63: ld (nn), hl
        &z80::neg, // $64: neg
        &z80::retn, // $65: retn
        &z80::nop,
        &z80::rrd, // $67: rrd
        &z80::in<AddressMode::RegisterL>, // $68: in l, (c)
        &z80::out<AddressMode::RegisterL>, // $69: out (c), l
        &z80::adc16<AddressMode::RegisterHL, AddressMode::RegisterHL>, // $6A: adc hl, hl
        &z80::ldRpAddr<AddressMode::RegisterHL>, // $6B: ld hl, (nn)
        &z80::neg, // $6C: neg
        &z80::retn, // $6D: retn
        &z80::nop,
        &z80::rld, // $6F: rld
        &z80::in<AddressMode::Flag>, // $70: in h, (c)
        &z80::out<AddressMode::Null>, // $71: out (c), 0
        &z80::sbc16<AddressMode::RegisterHL, AddressMode::RegisterSP>, // $72: sbc hl, sp
        &z80::ldAddrRp<AddressMode::RegisterSP>, // $73: ld (nn), sp
        &z80::neg, // $74: neg
        &z80::retn, // $75: retn
        &z80::nop, &z80::nop,
        &z80::in<AddressMode::Accumulator>, // $78: in a, (c)
        &z80::out<AddressMode::Accumulator>, // $79: out (c), a
        &z80::adc16<AddressMode::RegisterHL, AddressMode::RegisterSP>, // $7A: adc hl, sp
        &z80::ldRpAddr<AddressMode::RegisterSP>, // $7B: ld sp, (nn)
        &z80::neg, // $7C: neg
        &z80::retn, // $7D: reti
        &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::ldi, // $A0: ldi
        &z80::cpi, // $A1: cpi
        &z80::ini, // $A2: ini
        &z80::outi, // $A3: outi
        &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::ldd, // $A8: ldd
        &z80::cpd, // $A9: cpd
        &z80::ind, // $AA: ind
        &z80::outd, // $AB: outd
        &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::ldir, // $B0: ldir
        &z80::cpir, // $B1: cpir
        &z80::inir, // $B2: inir
        &z80::otir, // $B3: otir
        &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::lddr, // $B8: lddr
        &z80::cpdr, // $B9: cpdr
        &z80::indr, // $BA: indr
        &z80::otdr, // $BB: otdr
        &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
    };

    // Bit Instruction ($CB) Table
    static constexpr std::array<void (z80::*)(), 256> bitInstruction {
        &z80::rlc<AddressMode::RegisterB, AddressMode::RegisterB>, // $00: rlc b
        &z80::rlc<AddressMode::RegisterC, AddressMode::RegisterC>, // $01: rlc c
        &z80::rlc<AddressMode::RegisterD, AddressMode::RegisterD>, // $02: rlc d
        &z80::rlc<AddressMode::RegisterE, AddressMode::RegisterE>, // $03: rlc e
        &z80::rlc<AddressMode::RegisterH, AddressMode::RegisterH>, // $04: rlc h
        &z80::rlc<AddressMode::RegisterL, AddressMode::RegisterL>, // $05: rlc l
        &z80::rlc<AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $06: rlc (hl)
        &z80::rlc<AddressMode::Accumulator, AddressMode::Accumulator>, // $07: rlc a
        &z80::rrc<AddressMode::RegisterB, AddressMode::RegisterB>, // $08: rrc b
        &z80::rrc<AddressMode::RegisterC, AddressMode::RegisterC>, // $09: rrc c
        &z80::rrc<AddressMode::RegisterD, AddressMode::RegisterD>, // $0A: rrc d
        &z80::rrc<AddressMode::RegisterE, AddressMode::RegisterE>, // $0B: rrc e
        &z80::rrc<AddressMode::RegisterH, AddressMode::RegisterH>, // $0C: rrc h
        &z80::rrc<AddressMode::RegisterL, AddressMode::RegisterL>, // $0D: rrc l
        &z80::rrc<AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $0E: rrc (hl)
        &z80::rrc<AddressMode::Accumulator, AddressMode::Accumulator>, // $0F: rrc a
        &z80::rl<AddressMode::RegisterB, AddressMode::RegisterB>, // $10: rl b
        &z80::rl<AddressMode::RegisterC, AddressMode::RegisterC>, // $11: rl c
        &z80::rl<AddressMode::RegisterD, AddressMode::RegisterD>, // $12: rl d
        &z80::rl<AddressMode::RegisterE, AddressMode::RegisterE>, // $13: rl e
        &z80::rl<AddressMode::RegisterH, AddressMode::RegisterH>, // $14: rl h
        &z80::rl<AddressMode::RegisterL, AddressMode::RegisterL>, // $15: rl l
        &z80::rl<AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $16: rl (hl)
        &z80::rl<AddressMode::Accumulator, AddressMode::Accumulator>, // $17: rl a
        &z80::rr<AddressMode::RegisterB, AddressMode::RegisterB>, // $18: rr b
        &z80::rr<AddressMode::RegisterC, AddressMode::RegisterC>, // $19: rr c
        &z80::rr<AddressMode::RegisterD, AddressMode::RegisterD>, // $1A: rr d
        &z80::rr<AddressMode::RegisterE, AddressMode::RegisterE>, // $1B: rr e
        &z80::rr<AddressMode::RegisterH, AddressMode::RegisterH>, // $1C: rr h
        &z80::rr<AddressMode::RegisterL, AddressMode::RegisterL>, // $1D: rr l
        &z80::rr<AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $1E: rr (hl)
        &z80::rr<AddressMode::Accumulator, AddressMode::Accumulator>, // $1F: rr a
        &z80::sla<AddressMode::RegisterB, AddressMode::RegisterB>, // $20: sla b
        &z80::sla<AddressMode::RegisterC, AddressMode::RegisterC>, // $21: sla c
        &z80::sla<AddressMode::RegisterD, AddressMode::RegisterD>, // $22: sla d
        &z80::sla<AddressMode::RegisterE, AddressMode::RegisterE>, // $23: sla e
        &z80::sla<AddressMode::RegisterH, AddressMode::RegisterH>, // $24: sla h
        &z80::sla<AddressMode::RegisterL, AddressMode::RegisterL>, // $25: sla l
        &z80::sla<AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $26: sla (hl)
        &z80::sla<AddressMode::Accumulator, AddressMode::Accumulator>, // $27: sla a
        &z80::sra<AddressMode::RegisterB, AddressMode::RegisterB>, // $28: sra b
        &z80::sra<AddressMode::RegisterC, AddressMode::RegisterC>, // $29: sra c
        &z80::sra<AddressMode::RegisterD, AddressMode::RegisterD>, // $2A: sra d
        &z80::sra<AddressMode::RegisterE, AddressMode::RegisterE>, // $2B: sra e
        &z80::sra<AddressMode::RegisterH, AddressMode::RegisterH>, // $2C: sra h
        &z80::sra<AddressMode::RegisterL, AddressMode::RegisterL>, // $2D: sra l
        &z80::sra<AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $2E: sra (hl)
        &z80::sra<AddressMode::Accumulator, AddressMode::Accumulator>, // $2F: sra a
        &z80::sll<AddressMode::RegisterB, AddressMode::RegisterB>, // $30: sll b
        &z80::sll<AddressMode::RegisterC, AddressMode::RegisterC>, // $31: sll c
        &z80::sll<AddressMode::RegisterD, AddressMode::RegisterD>, // $32: sll d
        &z80::sll<AddressMode::RegisterE, AddressMode::RegisterE>, // $33: sll e
        &z80::sll<AddressMode::RegisterH, AddressMode::RegisterH>, // $34: sll h
        &z80::sll<AddressMode::RegisterL, AddressMode::RegisterL>, // $35: sll l
        &z80::sll<AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $36: sll (hl)
        &z80::sll<AddressMode::Accumulator, AddressMode::Accumulator>, // $37: sll a
        &z80::srl<AddressMode::RegisterB, AddressMode::RegisterB>, // $38: srl b
        &z80::srl<AddressMode::RegisterC, AddressMode::RegisterC>, // $39: srl c
        &z80::srl<AddressMode::RegisterD, AddressMode::RegisterD>, // $3A: srl d
        &z80::srl<AddressMode::RegisterE, AddressMode::RegisterE>, // $3B: srl e
        &z80::srl<AddressMode::RegisterH, AddressMode::RegisterH>, // $3C: srl h
        &z80::srl<AddressMode::RegisterL, AddressMode::RegisterL>, // $3D: srl l
        &z80::srl<AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $3E: srl (hl)
        &z80::srl<AddressMode::Accumulator, AddressMode::Accumulator>, // $3F: srl a
        &z80::bit<0x01U, AddressMode::RegisterB>, // $40: bit 0, b
        &z80::bit<0x01U, AddressMode::RegisterC>, // $41: bit 0, c
        &z80::bit<0x01U, AddressMode::RegisterD>, // $42: bit 0, d
        &z80::bit<0x01U, AddressMode::RegisterE>, // $43: bit 0, e
        &z80::bit<0x01U, AddressMode::RegisterH>, // $44: bit 0, h
        &z80::bit<0x01U, AddressMode::RegisterL>, // $45: bit 0, l
        &z80::bitHL<0x01U>, // $46: bit 0, (hl)
        &z80::bit<0x01U, AddressMode::Accumulator>, // $47: bit 0, a
        &z80::bit<0x02U, AddressMode::RegisterB>, // $48: bit 1, b
        &z80::bit<0x02U, AddressMode::RegisterC>, // $49: bit 1, c
        &z80::bit<0x02U, AddressMode::RegisterD>, // $4A: bit 1, d
        &z80::bit<0x02U, AddressMode::RegisterE>, // $4B: bit 1, e
        &z80::bit<0x02U, AddressMode::RegisterH>, // $4C: bit 1, h
        &z80::bit<0x02U, AddressMode::RegisterL>, // $4D: bit 1, l
        &z80::bitHL<0x02U>, // $4E: bit 1, (hl)
        &z80::bit<0x02U, AddressMode::Accumulator>, // $4F: bit 1, a
        &z80::bit<0x04U, AddressMode::RegisterB>, // $50: bit 2, b
        &z80::bit<0x04U, AddressMode::RegisterC>, // $51: bit 2, c
        &z80::bit<0x04U, AddressMode::RegisterD>, // $52: bit 2, d
        &z80::bit<0x04U, AddressMode::RegisterE>, // $53: bit 2, e
        &z80::bit<0x04U, AddressMode::RegisterH>, // $54: bit 2, h
        &z80::bit<0x04U, AddressMode::RegisterL>, // $55: bit 2, l
        &z80::bitHL<0x04U>, // $56: bit 2, (hl)
        &z80::bit<0x04U, AddressMode::Accumulator>, // $57: bit 2, a
        &z80::bit<0x08U, AddressMode::RegisterB>, // $58: bit 3, b
        &z80::bit<0x08U, AddressMode::RegisterC>, // $59: bit 3, c
        &z80::bit<0x08U, AddressMode::RegisterD>, // $5A: bit 3, d
        &z80::bit<0x08U, AddressMode::RegisterE>, // $5B: bit 3, e
        &z80::bit<0x08U, AddressMode::RegisterH>, // $5C: bit 3, h
        &z80::bit<0x08U, AddressMode::RegisterL>, // $5D: bit 3, l
        &z80::bitHL<0x08U>, // $5E: bit 3, (hl)
        &z80::bit<0x08U, AddressMode::Accumulator>, // $5F: bit 3, a
        &z80::bit<0x10U, AddressMode::RegisterB>, // $60: bit 4, b
        &z80::bit<0x10U, AddressMode::RegisterC>, // $61: bit 4, c
        &z80::bit<0x10U, AddressMode::RegisterD>, // $62: bit 4, d
        &z80::bit<0x10U, AddressMode::RegisterE>, // $63: bit 4, e
        &z80::bit<0x10U, AddressMode::RegisterH>, // $64: bit 4, h
        &z80::bit<0x10U, AddressMode::RegisterL>, // $65: bit 4, l
        &z80::bitHL<0x10>, // $66: bit 4, (hl)
        &z80::bit<0x10U, AddressMode::Accumulator>, // $67: bit 4, a
        &z80::bit<0x20U, AddressMode::RegisterB>, // $68: bit 5, b
        &z80::bit<0x20U, AddressMode::RegisterC>, // $69: bit 5, c
        &z80::bit<0x20U, AddressMode::RegisterD>, // $6A: bit 5, d
        &z80::bit<0x20U, AddressMode::RegisterE>, // $6B: bit 5, e
        &z80::bit<0x20U, AddressMode::RegisterH>, // $6C: bit 5, h
        &z80::bit<0x20U, AddressMode::RegisterL>, // $6D: bit 5, l
        &z80::bitHL<0x20U>, // $6E: bit 5, (hl)
        &z80::bit<0x20U, AddressMode::Accumulator>, // $6F: bit 5, a
        &z80::bit<0x40U, AddressMode::RegisterB>, // $70: bit 6, b
        &z80::bit<0x40U, AddressMode::RegisterC>, // $71: bit 6, c
        &z80::bit<0x40U, AddressMode::RegisterD>, // $72: bit 6, d
        &z80::bit<0x40U, AddressMode::RegisterE>, // $73: bit 6, e
        &z80::bit<0x40U, AddressMode::RegisterH>, // $74: bit 6, h
        &z80::bit<0x40U, AddressMode::RegisterL>, // $75: bit 6, l
        &z80::bitHL<0x40U>, // $76: bit 6, (hl)
        &z80::bit<0x40U, AddressMode::Accumulator>, // $77: bit 6, a
        &z80::bit<0x80U, AddressMode::RegisterB>, // $78: bit 7, b
        &z80::bit<0x80U, AddressMode::RegisterC>, // $79: bit 7, c
        &z80::bit<0x80U, AddressMode::RegisterD>, // $7A: bit 7, d
        &z80::bit<0x80U, AddressMode::RegisterE>, // $7B: bit 7, e
        &z80::bit<0x80U, AddressMode::RegisterH>, // $7C: bit 7, h
        &z80::bit<0x80U, AddressMode::RegisterL>, // $7D: bit 7, l
        &z80::bitHL<0x80U>, // $7E: bit 7, (hl)
        &z80::bit<0x80U, AddressMode::Accumulator>, // $7F: bit 7, a
        &z80::res<0xFEU, AddressMode::RegisterB, AddressMode::RegisterB>, // $80: res 0, b
        &z80::res<0xFEU, AddressMode::RegisterC, AddressMode::RegisterC>, // $81: res 0, c
        &z80::res<0xFEU, AddressMode::RegisterD, AddressMode::RegisterD>, // $82: res 0, d
        &z80::res<0xFEU, AddressMode::RegisterE, AddressMode::RegisterE>, // $83: res 0, e
        &z80::res<0xFEU, AddressMode::RegisterH, AddressMode::RegisterH>, // $84: res 0, h
        &z80::res<0xFEU, AddressMode::RegisterL, AddressMode::RegisterL>, // $85: res 0, l
        &z80::res<0xFEU, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $86: res 0, (hl)
        &z80::res<0xFEU, AddressMode::Accumulator, AddressMode::Accumulator>, // $87: res 0, a
        &z80::res<0xFDU, AddressMode::RegisterB, AddressMode::RegisterB>, // $88: res 1, b
        &z80::res<0xFDU, AddressMode::RegisterC, AddressMode::RegisterC>, // $89: res 1, c
        &z80::res<0xFDU, AddressMode::RegisterD, AddressMode::RegisterD>, // $8A: res 1, d
        &z80::res<0xFDU, AddressMode::RegisterE, AddressMode::RegisterE>, // $8B: res 1, e
        &z80::res<0xFDU, AddressMode::RegisterH, AddressMode::RegisterH>, // $8C: res 1, h
        &z80::res<0xFDU, AddressMode::RegisterL, AddressMode::RegisterL>, // $8D: res 1, l
        &z80::res<0xFDU, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $8E: res 1, (hl)
        &z80::res<0xFDU, AddressMode::Accumulator, AddressMode::Accumulator>, // $8F: res 1, a
        &z80::res<0xFBU, AddressMode::RegisterB, AddressMode::RegisterB>, // $90: res 2, b
        &z80::res<0xFBU, AddressMode::RegisterC, AddressMode::RegisterC>, // $91: res 2, c
        &z80::res<0xFBU, AddressMode::RegisterD, AddressMode::RegisterD>, // $92: res 2, d
        &z80::res<0xFBU, AddressMode::RegisterE, AddressMode::RegisterE>, // $93: res 2, e
        &z80::res<0xFBU, AddressMode::RegisterH, AddressMode::RegisterH>, // $94: res 2, h
        &z80::res<0xFBU, AddressMode::RegisterL, AddressMode::RegisterL>, // $95: res 2, l
        &z80::res<0xFBU, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $96: res 2, (hl)
        &z80::res<0xFBU, AddressMode::Accumulator, AddressMode::Accumulator>, // $97: res 2, a
        &z80::res<0xF7U, AddressMode::RegisterB, AddressMode::RegisterB>, // $98: res 3, b
        &z80::res<0xF7U, AddressMode::RegisterC, AddressMode::RegisterC>, // $99: res 3, c
        &z80::res<0xF7U, AddressMode::RegisterD, AddressMode::RegisterD>, // $9A: res 3, d
        &z80::res<0xF7U, AddressMode::RegisterE, AddressMode::RegisterE>, // $9B: res 3, e
        &z80::res<0xF7U, AddressMode::RegisterH, AddressMode::RegisterH>, // $9C: res 3, h
        &z80::res<0xF7U, AddressMode::RegisterL, AddressMode::RegisterL>, // $9D: res 3, l
        &z80::res<0xF7U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $9E: res 3, (hl)
        &z80::res<0xF7U, AddressMode::Accumulator, AddressMode::Accumulator>, // $9F: res 3, a
        &z80::res<0xEFU, AddressMode::RegisterB, AddressMode::RegisterB>, // $A0: res 4, b
        &z80::res<0xEFU, AddressMode::RegisterC, AddressMode::RegisterC>, // $A1: res 4, c
        &z80::res<0xEFU, AddressMode::RegisterD, AddressMode::RegisterD>, // $A2: res 4, d
        &z80::res<0xEFU, AddressMode::RegisterE, AddressMode::RegisterE>, // $A3: res 4, e
        &z80::res<0xEFU, AddressMode::RegisterH, AddressMode::RegisterH>, // $A4: res 4, h
        &z80::res<0xEFU, AddressMode::RegisterL, AddressMode::RegisterL>, // $A5: res 4, l
        &z80::res<0xEFU, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $A6: res 4, (hl)
        &z80::res<0xEFU, AddressMode::Accumulator, AddressMode::Accumulator>, // $A7: res 4, a
        &z80::res<0xDFU, AddressMode::RegisterB, AddressMode::RegisterB>, // $A8: res 5, b
        &z80::res<0xDFU, AddressMode::RegisterC, AddressMode::RegisterC>, // $A9: res 5, c
        &z80::res<0xDFU, AddressMode::RegisterD, AddressMode::RegisterD>, // $AA: res 5, d
        &z80::res<0xDFU, AddressMode::RegisterE, AddressMode::RegisterE>, // $AB: res 5, e
        &z80::res<0xDFU, AddressMode::RegisterH, AddressMode::RegisterH>, // $AC: res 5, h
        &z80::res<0xDFU, AddressMode::RegisterL, AddressMode::RegisterL>, // $AD: res 5, l
        &z80::res<0xDFU, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $AE: res 5, (hl)
        &z80::res<0xDFU, AddressMode::Accumulator, AddressMode::Accumulator>, // $AF: res 5, a
        &z80::res<0xBFU, AddressMode::RegisterB, AddressMode::RegisterB>, // $B0: res 6, b
        &z80::res<0xBFU, AddressMode::RegisterC, AddressMode::RegisterC>, // $B1: res 6, c
        &z80::res<0xBFU, AddressMode::RegisterD, AddressMode::RegisterD>, // $B2: res 6, d
        &z80::res<0xBFU, AddressMode::RegisterE, AddressMode::RegisterE>, // $B3: res 6, e
        &z80::res<0xBFU, AddressMode::RegisterH, AddressMode::RegisterH>, // $B4: res 6, h
        &z80::res<0xBFU, AddressMode::RegisterL, AddressMode::RegisterL>, // $B5: res 6, l
        &z80::res<0xBFU, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $B6: res 6, (hl)
        &z80::res<0xBFU, AddressMode::Accumulator, AddressMode::Accumulator>, // $B7: res 6, a
        &z80::res<0x7FU, AddressMode::RegisterB, AddressMode::RegisterB>, // $B8: res 7, b
        &z80::res<0x7FU, AddressMode::RegisterC, AddressMode::RegisterC>, // $B9: res 7, c
        &z80::res<0x7FU, AddressMode::RegisterD, AddressMode::RegisterD>, // $BA: res 7, d
        &z80::res<0x7FU, AddressMode::RegisterE, AddressMode::RegisterE>, // $BB: res 7, e
        &z80::res<0x7FU, AddressMode::RegisterH, AddressMode::RegisterH>, // $BC: res 7, h
        &z80::res<0x7FU, AddressMode::RegisterL, AddressMode::RegisterL>, // $BD: res 7, l
        &z80::res<0x7FU, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $BE: res 7, (hl)
        &z80::res<0x7FU, AddressMode::Accumulator, AddressMode::Accumulator>, // $BF: res 7, a
        &z80::set<0x01U, AddressMode::RegisterB, AddressMode::RegisterB>, // $C0: set 0, b
        &z80::set<0x01U, AddressMode::RegisterC, AddressMode::RegisterC>, // $C1: set 0, c
        &z80::set<0x01U, AddressMode::RegisterD, AddressMode::RegisterD>, // $C2: set 0, d
        &z80::set<0x01U, AddressMode::RegisterE, AddressMode::RegisterE>, // $C3: set 0, e
        &z80::set<0x01U, AddressMode::RegisterH, AddressMode::RegisterH>, // $C4: set 0, h
        &z80::set<0x01U, AddressMode::RegisterL, AddressMode::RegisterL>, // $C5: set 0, l
        &z80::set<0x01U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $C6: set 0, (hl)
        &z80::set<0x01U, AddressMode::Accumulator, AddressMode::Accumulator>, // $C7: set 0, a
        &z80::set<0x02U, AddressMode::RegisterB, AddressMode::RegisterB>, // $C8: set 1, b
        &z80::set<0x02U, AddressMode::RegisterC, AddressMode::RegisterC>, // $C9: set 1, c
        &z80::set<0x02U, AddressMode::RegisterD, AddressMode::RegisterD>, // $CA: set 1, d
        &z80::set<0x02U, AddressMode::RegisterE, AddressMode::RegisterE>, // $CB: set 1, e
        &z80::set<0x02U, AddressMode::RegisterH, AddressMode::RegisterH>, // $CC: set 1, h
        &z80::set<0x02U, AddressMode::RegisterL, AddressMode::RegisterL>, // $CD: set 1, l
        &z80::set<0x02U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $CE: set 1, (hl)
        &z80::set<0x02U, AddressMode::Accumulator, AddressMode::Accumulator>, // $CF: set 1, a
        &z80::set<0x04U, AddressMode::RegisterB, AddressMode::RegisterB>, // $D0: set 2, b
        &z80::set<0x04U, AddressMode::RegisterC, AddressMode::RegisterC>, // $D1: set 2, c
        &z80::set<0x04U, AddressMode::RegisterD, AddressMode::RegisterD>, // $D2: set 2, d
        &z80::set<0x04U, AddressMode::RegisterE, AddressMode::RegisterE>, // $D3: set 2, e
        &z80::set<0x04U, AddressMode::RegisterH, AddressMode::RegisterH>, // $D4: set 2, h
        &z80::set<0x04U, AddressMode::RegisterL, AddressMode::RegisterL>, // $D5: set 2, l
        &z80::set<0x04U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $D6: set 2, (hl)
        &z80::set<0x04U, AddressMode::Accumulator, AddressMode::Accumulator>, // $D7: set 2, a
        &z80::set<0x08U, AddressMode::RegisterB, AddressMode::RegisterB>, // $D8: set 3, b
        &z80::set<0x08U, AddressMode::RegisterC, AddressMode::RegisterC>, // $D9: set 3, c
        &z80::set<0x08U, AddressMode::RegisterD, AddressMode::RegisterD>, // $DA: set 3, d
        &z80::set<0x08U, AddressMode::RegisterE, AddressMode::RegisterE>, // $DB: set 3, e
        &z80::set<0x08U, AddressMode::RegisterH, AddressMode::RegisterH>, // $DC: set 3, h
        &z80::set<0x08U, AddressMode::RegisterL, AddressMode::RegisterL>, // $DD: set 3, l
        &z80::set<0x08U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $DE: set 3, (hl)
        &z80::set<0x08U, AddressMode::Accumulator, AddressMode::Accumulator>, // $DF: set 3, a
        &z80::set<0x10U, AddressMode::RegisterB, AddressMode::RegisterB>, // $E0: set 4, b
        &z80::set<0x10U, AddressMode::RegisterC, AddressMode::RegisterC>, // $E1: set 4, c
        &z80::set<0x10U, AddressMode::RegisterD, AddressMode::RegisterD>, // $E2: set 4, d
        &z80::set<0x10U, AddressMode::RegisterE, AddressMode::RegisterE>, // $E3: set 4, e
        &z80::set<0x10U, AddressMode::RegisterH, AddressMode::RegisterH>, // $E4: set 4, h
        &z80::set<0x10U, AddressMode::RegisterL, AddressMode::RegisterL>, // $E5: set 4, l
        &z80::set<0x10U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $E6: set 4, (hl)
        &z80::set<0x10U, AddressMode::Accumulator, AddressMode::Accumulator>, // $E7: set 4, a
        &z80::set<0x20U, AddressMode::RegisterB, AddressMode::RegisterB>, // $E8: set 5, b
        &z80::set<0x20U, AddressMode::RegisterC, AddressMode::RegisterC>, // $E9: set 5, c
        &z80::set<0x20U, AddressMode::RegisterD, AddressMode::RegisterD>, // $EA: set 5, d
        &z80::set<0x20U, AddressMode::RegisterE, AddressMode::RegisterE>, // $EB: set 5, e
        &z80::set<0x20U, AddressMode::RegisterH, AddressMode::RegisterH>, // $EC: set 5, h
        &z80::set<0x20U, AddressMode::RegisterL, AddressMode::RegisterL>, // $ED: set 5, l
        &z80::set<0x20U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $EE: set 5, (hl)
        &z80::set<0x20U, AddressMode::Accumulator, AddressMode::Accumulator>, // $EF: set 5, a
        &z80::set<0x40U, AddressMode::RegisterB, AddressMode::RegisterB>, // $F0: set 6, b
        &z80::set<0x40U, AddressMode::RegisterC, AddressMode::RegisterC>, // $F1: set 6, c
        &z80::set<0x40U, AddressMode::RegisterD, AddressMode::RegisterD>, // $F2: set 6, d
        &z80::set<0x40U, AddressMode::RegisterE, AddressMode::RegisterE>, // $F3: set 6, e
        &z80::set<0x40U, AddressMode::RegisterH, AddressMode::RegisterH>, // $F4: set 6, h
        &z80::set<0x40U, AddressMode::RegisterL, AddressMode::RegisterL>, // $F5: set 6, l
        &z80::set<0x40U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $F6: set 6, (hl)
        &z80::set<0x40U, AddressMode::Accumulator, AddressMode::Accumulator>, // $F7: set 6, a
        &z80::set<0x80U, AddressMode::RegisterB, AddressMode::RegisterB>, // $F8: set 7, b
        &z80::set<0x80U, AddressMode::RegisterC, AddressMode::RegisterC>, // $F9: set 7, c
        &z80::set<0x80U, AddressMode::RegisterD, AddressMode::RegisterD>, // $FA: set 7, d
        &z80::set<0x80U, AddressMode::RegisterE, AddressMode::RegisterE>, // $FB: set 7, e
        &z80::set<0x80U, AddressMode::RegisterH, AddressMode::RegisterH>, // $FC: set 7, h
        &z80::set<0x80U, AddressMode::RegisterL, AddressMode::RegisterL>, // $FD: set 7, l
        &z80::set<0x80U, AddressMode::RegisterIndirectHL, AddressMode::RegisterIndirectHL>, // $FE: set 7, (hl)
        &z80::set<0x80U, AddressMode::Accumulator, AddressMode::Accumulator>, // $FF: set 7, a
    };

    // Index Instruction ($DD/$FD) Table
    template<AddressMode mode, AddressMode modeH, AddressMode modeL, AddressMode modeD>
    static constexpr std::array<void (z80::*)(), 256> indexInstruction {
        &z80::nop, // $00: nop
        &z80::ld<AddressMode::RegisterBC, AddressMode::ImmediateEx>, // $01: ld bc, nn
        &z80::ld<AddressMode::RegisterIndirectBC, AddressMode::Accumulator>, // $02: ld (bc), a
        &z80::inc16<AddressMode::RegisterBC>, // $03: inc bc
        &z80::inc8<AddressMode::RegisterB>, // $04: inc b
        &z80::dec8<AddressMode::RegisterB>, // $05: dec b
        &z80::ld<AddressMode::RegisterB, AddressMode::Immediate>, // $06: ld b, n
        &z80::rlca, // $07: rlca,
        &z80::ex1, // $08: ex af, af'
        &z80::add16<mode, AddressMode::RegisterBC>, // $09: add ix/iy, bc
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterIndirectBC>, // $0A: ld a, (bc)
        &z80::dec16<AddressMode::RegisterBC>, // $0B: dec bc
        &z80::inc8<AddressMode::RegisterC>, // $0C: inc c
        &z80::dec8<AddressMode::RegisterC>, // $0D: dec c
        &z80::ld<AddressMode::RegisterC, AddressMode::Immediate>, // $0E: ld c, n
        &z80::rrca, // $0F: rrca
        &z80::djnz, // $10: djnz d
        &z80::ld<AddressMode::RegisterDE, AddressMode::ImmediateEx>, // $11: ld de, nn
        &z80::ld<AddressMode::RegisterIndirectDE, AddressMode::Accumulator>, // $12: ld (de), a
        &z80::inc16<AddressMode::RegisterDE>, // $13: inc de
        &z80::inc8<AddressMode::RegisterD>, // $14: inc d
        &z80::dec8<AddressMode::RegisterD>, // $15: dec d
        &z80::ld<AddressMode::RegisterD, AddressMode::Immediate>, // $16: ld d, n
        &z80::rla, // $17: rla
        &z80::jr<Condition::Null>, // $18: jr d
        &z80::add16<mode, AddressMode::RegisterDE>, // $19: add ix/iy, de
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterIndirectDE>, // $1A: ld a, (de)
        &z80::dec16<AddressMode::RegisterDE>, // $1B: dec de
        &z80::inc8<AddressMode::RegisterE>, // $1C: inc e
        &z80::dec8<AddressMode::RegisterE>, // $1D: dec e
        &z80::ld<AddressMode::RegisterE, AddressMode::Immediate>, // $1E: ld e, n
        &z80::rra, // $1F: rra
        &z80::jr<Condition::NZ>, // $20: jr nz, d
        &z80::ld<mode, AddressMode::ImmediateEx>, // $21: ld ix/iy, nn
        &z80::ldAddrRp<mode>, // $22: ld (nn), ix/iy
        &z80::inc16<mode>, // $23: inc ix/iy
        &z80::inc8<modeH>, // $24: inc ixh/iyh
        &z80::dec8<modeH>, // $25: dec ixh/iyh
        &z80::ld<modeH, AddressMode::Immediate>, // $26: ld ixh/iyh, n
        &z80::daa, // $27: daa
        &z80::jr<Condition::Z>, // $28: jr z, d
        &z80::add16<mode, mode>, // $29: add ix/iy, ix/iy
        &z80::ldRpAddr<mode>, // $2A: ld ix/iy, (nn)
        &z80::dec16<mode>, // $2B: dec ix/iy
        &z80::inc8<modeL>, // $2C: inc ixl/iyl
        &z80::dec8<modeL>, // $2D: dec ixl/iyl
        &z80::ld<modeL, AddressMode::Immediate>, // $2E: ld ixl/iyl, n
        &z80::cpl, // $2F: cpl
        &z80::jr<Condition::NC>, // $30: jr nc, d
        &z80::ld<AddressMode::RegisterSP, AddressMode::ImmediateEx>, // $31 ld sp, nn
        &z80::ld<AddressMode::Extended, AddressMode::Accumulator>, // $32: ld (nn), a
        &z80::inc16<AddressMode::RegisterSP>, // $33: inc sp
        &z80::inc8<modeD>, // $34: inc (ix+d/iy+d)
        &z80::dec8<modeD>, // $35: dec (ix+d/iy+d)
        &z80::ld<modeD, AddressMode::Immediate>, // $36: ld (ix+d/iy+d), n
        &z80::scf, // $37: scf
        &z80::jr<Condition::C>, // $38: jr c, d
        &z80::add16<mode, AddressMode::RegisterSP>, // $39: add ix/iy, sp
        &z80::ld<AddressMode::Accumulator, AddressMode::Extended>, // $3A: ld a, (nn)
        &z80::dec16<AddressMode::RegisterSP>, // $3B: dec sp
        &z80::inc8<AddressMode::Accumulator>, // $3C: inc a
        &z80::dec8<AddressMode::Accumulator>, // $3D: dec a
        &z80::ld<AddressMode::Accumulator, AddressMode::Immediate>, // $3E: ld a, n
        &z80::ccf, // $3F: ccf
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterB>, // $40: ld b, b
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterC>, // $41: ld b, c
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterD>, // $42: ld b, d
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterE>, // $43: ld b, e
        &z80::ld<AddressMode::RegisterB, modeH>, // $44: ld b, ixh/iyh
        &z80::ld<AddressMode::RegisterB, modeL>, // $45: ld b, ixl/iyl
        &z80::ld<AddressMode::RegisterB, modeD>, // $46: ld b, (ix+d/iy+d)
        &z80::ld<AddressMode::RegisterB, AddressMode::Accumulator>, // $47: ld c, a
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterB>, // $48: ld c, b
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterC>, // $49: ld c, c
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterD>, // $4A: ld c, d
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterE>, // $4B: ld c, e
        &z80::ld<AddressMode::RegisterC, modeH>, // $4C: ld c, ixh/iyh
        &z80::ld<AddressMode::RegisterC, modeL>, // $4D: ld c, ixl/iyl
        &z80::ld<AddressMode::RegisterC, modeD>, // $4E: ld c, (ix+d/iy+d)
        &z80::ld<AddressMode::RegisterC, AddressMode::Accumulator>, // $4F: ld c, a
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterB>, // $50: ld d, b
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterC>, // $51: ld d, c
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterD>, // $52: ld d, d
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterE>, // $53: ld d, e
        &z80::ld<AddressMode::RegisterD, modeH>, // $54: ld d, ixh/iyh
        &z80::ld<AddressMode::RegisterD, modeL>, // $55: ld d, ixl/iyl
        &z80::ld<AddressMode::RegisterD, modeD>, // $56: ld d, (ix+d/iy+d)
        &z80::ld<AddressMode::RegisterD, AddressMode::Accumulator>, // $57: ld d, a
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterB>, // $58: ld e, b
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterC>, // $59: ld e, c
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterD>, // $5A: ld e, d
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterE>, // $5B: ld e, e
        &z80::ld<AddressMode::RegisterE, modeH>, // $5C: ld e, ixh/iyh
        &z80::ld<AddressMode::RegisterE, modeL>, // $5D: ld e, ixl/iyl
        &z80::ld<AddressMode::RegisterE, modeD>, // $5E: ld e, (ix+d/iy+d)
        &z80::ld<AddressMode::RegisterE, AddressMode::Accumulator>, // $5F: ld e, a
        &z80::ld<modeH, AddressMode::RegisterB>, // $60: ld ixh/iyh, b
        &z80::ld<modeH, AddressMode::RegisterC>, // $61: ld ixh/iyh, c
        &z80::ld<modeH, AddressMode::RegisterD>, // $62: ld ixh/iyh, d
        &z80::ld<modeH, AddressMode::RegisterE>, // $63: ld ixh/iyh, e
        &z80::ld<modeH, modeH>, // $64: ld ixh/iyh, ixh/iyh
        &z80::ld<modeH, modeL>, // $65: ld ixh/iyh, ixl/iyl
        &z80::ld<AddressMode::RegisterH, modeD>, // $66: ld h, (ix+d/iy+d)
        &z80::ld<modeH, AddressMode::Accumulator>, // $67: ld ixh/iyh, a
        &z80::ld<modeL, AddressMode::RegisterB>, // $68: ld ixl/iyl, b
        &z80::ld<modeL, AddressMode::RegisterC>, // $69: ld ixl/iyl, c
        &z80::ld<modeL, AddressMode::RegisterD>, // $6A: ld ixl/iyl, d
        &z80::ld<modeL, AddressMode::RegisterE>, // $6B: ld ixl/iyl, e
        &z80::ld<modeL, modeH>, // $6C: ld ixl/iyl, ixh/iyh
        &z80::ld<modeL, modeL>, // $6D: ld ixl/iyl, ixl/iyl
        &z80::ld<AddressMode::RegisterL, modeD>, // $6E: ld l, (ix+d/iy+d)
        &z80::ld<modeL, AddressMode::Accumulator>, // $6F: ld (ix+d/iy+d), a
        &z80::ld<modeD, AddressMode::RegisterB>, // $70: ld (ix+d/iy+d), b
        &z80::ld<modeD, AddressMode::RegisterC>, // $71: ld (ix+d/iy+d), c
        &z80::ld<modeD, AddressMode::RegisterD>, // $72: ld (ix+d/iy+d), d
        &z80::ld<modeD, AddressMode::RegisterE>, // $73: ld (ix+d/iy+d), e
        &z80::ld<modeD, AddressMode::RegisterH>, // $74: ld (ix+d/iy+d), h
        &z80::ld<modeD, AddressMode::RegisterL>, // $75: ld (ix+d/iy+d), l
        &z80::halt, // $76: halt
        &z80::ld<modeD, AddressMode::Accumulator>, // $77: ld (ix+d/iy+d), a
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterB>, // $78: ld a, b
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterC>, // $79: ld a, c
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterD>, // $7A: ld a, d
        &z80::ld<AddressMode::Accumulator, AddressMode::RegisterE>, // $7B: ld a, e
        &z80::ld<AddressMode::Accumulator, modeH>, // $7C: ld a, ixh/iyh
        &z80::ld<AddressMode::Accumulator, modeL>, // $7D: ld a, ixl/iyl
        &z80::ld<AddressMode::Accumulator, modeD>, // $7E: ld a, (ix+d/iy+d)
        &z80::ld<AddressMode::Accumulator, AddressMode::Accumulator>, // $7F: ld a, a
        &z80::add8<AddressMode::RegisterB>, // $80: add a, b
        &z80::add8<AddressMode::RegisterC>, // $81: add a, c
        &z80::add8<AddressMode::RegisterD>, // $82: add a, d
        &z80::add8<AddressMode::RegisterE>, // $83: add a, e
        &z80::add8<modeH>, // $84: add a, ixh/iyh
        &z80::add8<modeL>, // $85: add a, ixl/iyl
        &z80::add8<modeD>, // $86: add a, (ix+d/iy+d)
        &z80::add8<AddressMode::Accumulator>, // $87: add a, a
        &z80::adc8<AddressMode::RegisterB>, // $88: adc a, b
        &z80::adc8<AddressMode::RegisterC>, // $89: adc a, c
        &z80::adc8<AddressMode::RegisterD>, // $8A: adc a, d
        &z80::adc8<AddressMode::RegisterE>, // $8B: adc a, e
        &z80::adc8<modeH>, // $8C: adc a, ixh/iyh
        &z80::adc8<modeL>, // $8D: adc a, ixl/iyl
        &z80::adc8<modeD>, // $8E: adc a, (ix+d/iy+d)
        &z80::adc8<AddressMode::Accumulator>, // $8F: adc a, a
        &z80::sub<AddressMode::RegisterB>, // $90: sub b
        &z80::sub<AddressMode::RegisterC>, // $91: sub c
        &z80::sub<AddressMode::RegisterD>, // $92: sub d
        &z80::sub<AddressMode::RegisterE>, // $93: sub e
        &z80::sub<modeH>, // $94: sub ixh/iyh
        &z80::sub<modeL>, // $95: sub ixl/iyl
        &z80::sub<modeD>, // $96: sub (ix+d/iy+d)
        &z80::sub<AddressMode::Accumulator>, // $97: sub a
        &z80::sbc8<AddressMode::RegisterB>, // $98: sbc a, b
        &z80::sbc8<AddressMode::RegisterC>, // $99: sbc a, c
        &z80::sbc8<AddressMode::RegisterD>, // $9A: sbc a, d
        &z80::sbc8<AddressMode::RegisterE>, // $9B: sbc a, e
        &z80::sbc8<modeH>, // $9C: sbc a, ixh/iyh
        &z80::sbc8<modeL>, // $9D: sbc a, ixl/iyl
        &z80::sbc8<modeD>, // $9E: sbc a, (ix+d/iy+d)
        &z80::sbc8<AddressMode::Accumulator>, // $AF: sbc a, a
        &z80::land<AddressMode::RegisterB>, // $A0: and b
        &z80::land<AddressMode::RegisterC>, // $A1: and c
        &z80::land<AddressMode::RegisterD>, // $A2: and d
        &z80::land<AddressMode::RegisterE>, // $A3: and e
        &z80::land<modeH>, // $A4: and ixh/iyh
        &z80::land<modeL>, // $A5: and ixl/iyl
        &z80::land<modeD>, // $A6: and (ix+d/iy+d)
        &z80::land<AddressMode::Accumulator>, // $A7: and a
        &z80::lxor<AddressMode::RegisterB>, // $A8: xor b
        &z80::lxor<AddressMode::RegisterC>, // $A9: xor c
        &z80::lxor<AddressMode::RegisterD>, // $AA: xor d
        &z80::lxor<AddressMode::RegisterE>, // $AB: xor e
        &z80::lxor<modeH>, // $AC: xor ixh/iyh
        &z80::lxor<modeL>, // $AD: xor ixl/iyl
        &z80::lxor<modeD>, // $AE: xor a, (ix+d/iy+d)
        &z80::lxor<AddressMode::Accumulator>, // $AF: xor a
        &z80::lor<AddressMode::RegisterB>, // $B0: or b
        &z80::lor<AddressMode::RegisterC>, // $B1: or c
        &z80::lor<AddressMode::RegisterD>, // $B2: or d
        &z80::lor<AddressMode::RegisterE>, // $B3: or e
        &z80::lor<modeH>, // $B4: or ixh/iyh
        &z80::lor<modeL>, // $B5: or ixl/iyl
        &z80::lor<modeD>, // $B6: or (ix+d/iy+d)
        &z80::lor<AddressMode::Accumulator>, // $B7: or a
        &z80::cp<AddressMode::RegisterB>, // $B8: cp b
        &z80::cp<AddressMode::RegisterC>, // $B9: cp c
        &z80::cp<AddressMode::RegisterD>, // $BA: cp d
        &z80::cp<AddressMode::RegisterE>, // $BB: cp e
        &z80::cp<modeH>, // $BC: cp ixh/iyh
        &z80::cp<modeL>, // $BD: cp ixl/iyl
        &z80::cp<modeD>, // $BE: cp (ix+d/iy+d)
        &z80::cp<AddressMode::Accumulator>, // $BF: cp a
        &z80::ret<Condition::NZ>, // $C0: ret nz
        &z80::pop<AddressMode::RegisterBC>, // $C1: pop bc
        &z80::jp<Condition::NZ, AddressMode::ImmediateEx>, // $C2: jp nz, nn
        &z80::jp<Condition::Null, AddressMode::ImmediateEx>, // $C3: jp nn
        &z80::call<Condition::NZ, AddressMode::ImmediateEx>, // $C4: call nz, nn
        &z80::push<AddressMode::RegisterBC>, // $C5: push bc
        &z80::add8<AddressMode::Immediate>, // $C6: add a, n
        &z80::rst<0x00U>, // $C7: rst 0
        &z80::ret<Condition::Z>, // $C8: ret z
        &z80::ret<Condition::Null>, // $C9: ret
        &z80::jp<Condition::Z, AddressMode::ImmediateEx>, // $CA: jp z, nn
        &z80::preIndexBit<modeD>, // $CB: index bit
        &z80::call<Condition::Z, AddressMode::ImmediateEx>, // $CC: call z, nn
        &z80::call<Condition::Null, AddressMode::ImmediateEx>, // $CD: call nn
        &z80::adc8<AddressMode::Immediate>, // $CE: adc a, n
        &z80::rst<0x08U>, // $CF: rst 8
        &z80::ret<Condition::NC>, // $D0: ret nc
        &z80::pop<AddressMode::RegisterDE>, // $D1: pop de
        &z80::jp<Condition::NC, AddressMode::ImmediateEx>, // $D2: jp nc, nn
        &z80::out, // $D3: out (n), a
        &z80::call<Condition::NC, AddressMode::ImmediateEx>, // $D4: call nc, nn
        &z80::push<AddressMode::RegisterDE>, // $D5: push de
        &z80::sub<AddressMode::Immediate>, // $D6: sub n
        &z80::rst<0x10U>, // $D7: rst 16
        &z80::ret<Condition::C>, // $D8: ret c
        &z80::exx, // $D9: exx
        &z80::jp<Condition::C, AddressMode::ImmediateEx>, // $DA: jp c, nn
        &z80::in, // $DB: in a, (n)
        &z80::call<Condition::C, AddressMode::ImmediateEx>, // $DC: call c, nn
        &z80::preIndexBit<modeD>, // $DD: IX
        &z80::sbc8<AddressMode::Immediate>, // $DE: sbc a, n
        &z80::rst<0x18U>, // $DF: rst 24
        &z80::ret<Condition::PO>, // $E0: ret po
        &z80::pop<mode>, // $E1: pop ix/iy
        &z80::jp<Condition::PO, AddressMode::ImmediateEx>, // $E2: jp po, nn
        &z80::ex2<mode>, // $E3: ex (sp), ix/iy
        &z80::call<Condition::PO, AddressMode::ImmediateEx>, // $E4: call po, nn
        &z80::push<mode>, // $E5: push ix/iy
        &z80::land<AddressMode::Immediate>, // $E6: add n
        &z80::rst<0x20U>, // $E7: rst 32
        &z80::ret<Condition::PE>, // $E8: ret pe
        &z80::jp<Condition::Null, mode>, // $E9: jp (ix/iy)
        &z80::jp<Condition::PE, AddressMode::ImmediateEx>, // $EA: jp pe, nn
        &z80::ex3, // $EB: ex de, hl
        &z80::call<Condition::PE, AddressMode::ImmediateEx>, // $EC: call pe, nn
        &z80::nop, // $ED: nop
        &z80::lxor<AddressMode::Immediate>, // $EE: xor n
        &z80::rst<0x28U>, // $EF: rst 40
        &z80::ret<Condition::P>, // $F0: ret p
        &z80::pop<AddressMode::RegisterAF>, // $F1: pop af
        &z80::jp<Condition::P, AddressMode::ImmediateEx>, // $F2: jp p, nn
        &z80::di, // $F3: di
        &z80::call<Condition::P, AddressMode::ImmediateEx>, // $F4: call p, nn
        &z80::push<AddressMode::RegisterAF>, // $F5: push af
        &z80::lor<AddressMode::Immediate>, // $F6: or n
        &z80::rst<0x30U>, // $F7: rst 48
        &z80::ret<Condition::M>, // $F8: ret m
        &z80::ld<AddressMode::RegisterSP, mode>, // $F9: ld sp, ix/iy
        &z80::jp<Condition::M, AddressMode::ImmediateEx>, // $FA: jp m, nn
        &z80::ei, // $FB: ei
        &z80::call<Condition::M, AddressMode::ImmediateEx>, // $FC: call m, nn
        &z80::nop, // $FD: nop
        &z80::cp<AddressMode::Immediate>, // $FE: cp n
        &z80::rst<0x38U>,// $FF: rst 56
    };

    // Index Bit instruction ($DDCB/$FDCB) Table
    template<AddressMode mode>
    static constexpr std::array<void (z80::*)(), 256> indexBitInstruction {
        &z80::rlc<mode, AddressMode::RegisterB>, // $00: rlc (ix+d/iy+d), b
        &z80::rlc<mode, AddressMode::RegisterC>, // $01: rlc (ix+d/iy+d), c
        &z80::rlc<mode, AddressMode::RegisterD>, // $02: rlc (ix+d/iy+d), d
        &z80::rlc<mode, AddressMode::RegisterE>, // $03: rlc (ix+d/iy+d), e
        &z80::rlc<mode, AddressMode::RegisterH>, // $04: rlc (ix+d/iy+d), h
        &z80::rlc<mode, AddressMode::RegisterL>, // $05: rlc (ix+d/iy+d), l
        &z80::rlc<mode, mode>, // $06: rlc (ix+d/iy+d)
        &z80::rlc<mode, AddressMode::Accumulator>, // $07: rlc (ix+d/iy+d), a
        &z80::rrc<mode, AddressMode::RegisterB>, // $08: rrc (ix+d/iy+d), b
        &z80::rrc<mode, AddressMode::RegisterC>, // $09: rrc (ix+d/iy+d), c
        &z80::rrc<mode, AddressMode::RegisterD>, // $0A: rrc (ix+d/iy+d), d
        &z80::rrc<mode, AddressMode::RegisterE>, // $0B: rrc (ix+d/iy+d), e
        &z80::rrc<mode, AddressMode::RegisterH>, // $0C: rrc (ix+d/iy+d), h
        &z80::rrc<mode, AddressMode::RegisterL>, // $0D: rrc (ix+d/iy+d), l
        &z80::rrc<mode, mode>, // $0E: rrc (ix+d/iy+d)
        &z80::rrc<mode, AddressMode::Accumulator>, // $0F: rrc (ix+d/iy+d), a
        &z80::rl<mode, AddressMode::RegisterB>, // $10: rl (ix+d/iy+d), b
        &z80::rl<mode, AddressMode::RegisterC>, // $11: rl (ix+d/iy+d), c
        &z80::rl<mode, AddressMode::RegisterD>, // $12: rl (ix+d/iy+d), d
        &z80::rl<mode, AddressMode::RegisterE>, // $13: rl (ix+d/iy+d), e
        &z80::rl<mode, AddressMode::RegisterH>, // $14: rl (ix+d/iy+d), h
        &z80::rl<mode, AddressMode::RegisterL>, // $15: rl (ix+d/iy+d), l
        &z80::rl<mode, mode>, // $16: rl (ix+d/iy+d)
        &z80::rl<mode, AddressMode::Accumulator>, // $17: rl (ix+d/iy+d), a
        &z80::rr<mode, AddressMode::RegisterB>, // $18: rr (ix+d/iy+d), b
        &z80::rr<mode, AddressMode::RegisterC>, // $19: rr (ix+d/iy+d), c
        &z80::rr<mode, AddressMode::RegisterD>, // $1A: rr (ix+d/iy+d), d
        &z80::rr<mode, AddressMode::RegisterE>, // $1B: rr (ix+d/iy+d), e
        &z80::rr<mode, AddressMode::RegisterH>, // $1C: rr (ix+d/iy+d), h
        &z80::rr<mode, AddressMode::RegisterL>, // $1D: rr (ix+d/iy+d), l
        &z80::rr<mode, mode>, // $1E: rr (ix+d/iy+d)
        &z80::rr<mode, AddressMode::Accumulator>, // $1F: rr (ix+d/iy+d), a
        &z80::sla<mode, AddressMode::RegisterB>, // $20: sla (ix+d/iy+d), b
        &z80::sla<mode, AddressMode::RegisterC>, // $21: sla (ix+d/iy+d), c
        &z80::sla<mode, AddressMode::RegisterD>, // $22: sla (ix+d/iy+d), d
        &z80::sla<mode, AddressMode::RegisterE>, // $23: sla (ix+d/iy+d), e
        &z80::sla<mode, AddressMode::RegisterH>, // $24: sla (ix+d/iy+d), h
        &z80::sla<mode, AddressMode::RegisterL>, // $25: sla (ix+d/iy+d), l
        &z80::sla<mode, mode>, // $26: sla (ix+d/iy+d)
        &z80::sla<mode, AddressMode::Accumulator>, // $27: sla (ix+d/iy+d), a
        &z80::sra<mode, AddressMode::RegisterB>, // $28: sra (ix+d/iy+d), b
        &z80::sra<mode, AddressMode::RegisterC>, // $29: sra (ix+d/iy+d), c
        &z80::sra<mode, AddressMode::RegisterD>, // $2A: sra (ix+d/iy+d), d
        &z80::sra<mode, AddressMode::RegisterE>, // $2B: sra (ix+d/iy+d), e
        &z80::sra<mode, AddressMode::RegisterH>, // $2C: sra (ix+d/iy+d), h
        &z80::sra<mode, AddressMode::RegisterL>, // $2D: sra (ix+d/iy+d), l
        &z80::sra<mode, mode>, // $2E: sra (ix+d/iy+d)
        &z80::sra<mode, AddressMode::Accumulator>, // $2F: sra (ix+d/iy+d), a
        &z80::sll<mode, AddressMode::RegisterB>, // $30: sll (ix+d/iy+d), b
        &z80::sll<mode, AddressMode::RegisterC>, // $31: sll (ix+d/iy+d), c
        &z80::sll<mode, AddressMode::RegisterD>, // $32: sll (ix+d/iy+d), d
        &z80::sll<mode, AddressMode::RegisterE>, // $33: sll (ix+d/iy+d), e
        &z80::sll<mode, AddressMode::RegisterH>, // $34: sll (ix+d/iy+d), h
        &z80::sll<mode, AddressMode::RegisterL>, // $35: sll (ix+d/iy+d), l
        &z80::sll<mode, mode>, // $36: sll (ix+d/iy+d)
        &z80::sll<mode, AddressMode::Accumulator>, // $37: sll (ix+d/iy+d), a
        &z80::srl<mode, AddressMode::RegisterB>, // $38: srl (ix+d/iy+d), b
        &z80::srl<mode, AddressMode::RegisterC>, // $39: srl (ix+d/iy+d), c
        &z80::srl<mode, AddressMode::RegisterD>, // $3A: srl (ix+d/iy+d), d
        &z80::srl<mode, AddressMode::RegisterE>, // $3B: srl (ix+d/iy+d), e
        &z80::srl<mode, AddressMode::RegisterH>, // $3C: srl (ix+d/iy+d), h
        &z80::srl<mode, AddressMode::RegisterL>, // $3D: srl (ix+d/iy+d), l
        &z80::srl<mode, mode>, // $3E: srl (ix+d/iy+d)
        &z80::srl<mode, AddressMode::Accumulator>, // $3F: srl (ix+d/iy+d), a
        &z80::bit<0x01U, mode>, // $40: bit 0, (ix+d/iy+d)
        &z80::bit<0x01U, mode>, // $41: bit 0, (ix+d/iy+d)
        &z80::bit<0x01U, mode>, // $42: bit 0, (ix+d/iy+d)
        &z80::bit<0x01U, mode>, // $43: bit 0, (ix+d/iy+d)
        &z80::bit<0x01U, mode>, // $44: bit 0, (ix+d/iy+d)
        &z80::bit<0x01U, mode>, // $45: bit 0, (ix+d/iy+d)
        &z80::bit<0x01U, mode>, // $46: bit 0, (ix+d/iy+d)
        &z80::bit<0x01U, mode>, // $47: bit 0, (ix+d/iy+d)
        &z80::bit<0x02U, mode>, // $48: bit 1, (ix+d/iy+d)
        &z80::bit<0x02U, mode>, // $49: bit 1, (ix+d/iy+d)
        &z80::bit<0x02U, mode>, // $4A: bit 1, (ix+d/iy+d)
        &z80::bit<0x02U, mode>, // $4B: bit 1, (ix+d/iy+d)
        &z80::bit<0x02U, mode>, // $4C: bit 1, (ix+d/iy+d)
        &z80::bit<0x02U, mode>, // $4D: bit 1, (ix+d/iy+d)
        &z80::bit<0x02U, mode>, // $4E: bit 1, (ix+d/iy+d)
        &z80::bit<0x02U, mode>, // $4F: bit 1, (ix+d/iy+d)
        &z80::bit<0x04U, mode>, // $50: bit 2, (ix+d/iy+d)
        &z80::bit<0x04U, mode>, // $51: bit 2, (ix+d/iy+d)
        &z80::bit<0x04U, mode>, // $52: bit 2, (ix+d/iy+d)
        &z80::bit<0x04U, mode>, // $53: bit 2, (ix+d/iy+d)
        &z80::bit<0x04U, mode>, // $54: bit 2, (ix+d/iy+d)
        &z80::bit<0x04U, mode>, // $55: bit 2, (ix+d/iy+d)
        &z80::bit<0x04U, mode>, // $56: bit 2, (ix+d/iy+d)
        &z80::bit<0x04U, mode>, // $57: bit 2, (ix+d/iy+d)
        &z80::bit<0x08U, mode>, // $58: bit 3, (ix+d/iy+d)
        &z80::bit<0x08U, mode>, // $59: bit 3, (ix+d/iy+d)
        &z80::bit<0x08U, mode>, // $5A: bit 3, (ix+d/iy+d)
        &z80::bit<0x08U, mode>, // $5B: bit 3, (ix+d/iy+d)
        &z80::bit<0x08U, mode>, // $5C: bit 3, (ix+d/iy+d)
        &z80::bit<0x08U, mode>, // $5D: bit 3, (ix+d/iy+d)
        &z80::bit<0x08U, mode>, // $5E: bit 3, (ix+d/iy+d)
        &z80::bit<0x08U, mode>, // $5F: bit 3, (ix+d/iy+d)
        &z80::bit<0x10U, mode>, // $60: bit 4, (ix+d/iy+d)
        &z80::bit<0x10U, mode>, // $61: bit 4, (ix+d/iy+d)
        &z80::bit<0x10U, mode>, // $62: bit 4, (ix+d/iy+d)
        &z80::bit<0x10U, mode>, // $63: bit 4, (ix+d/iy+d)
        &z80::bit<0x10U, mode>, // $64: bit 4, (ix+d/iy+d)
        &z80::bit<0x10U, mode>, // $65: bit 4, (ix+d/iy+d)
        &z80::bit<0x10U, mode>, // $66: bit 4, (ix+d/iy+d)
        &z80::bit<0x10U, mode>, // $67: bit 4, (ix+d/iy+d)
        &z80::bit<0x20U, mode>, // $68: bit 5, (ix+d/iy+d)
        &z80::bit<0x20U, mode>, // $69: bit 5, (ix+d/iy+d)
        &z80::bit<0x20U, mode>, // $6A: bit 5, (ix+d/iy+d)
        &z80::bit<0x20U, mode>, // $6B: bit 5, (ix+d/iy+d)
        &z80::bit<0x20U, mode>, // $6C: bit 5, (ix+d/iy+d)
        &z80::bit<0x20U, mode>, // $6D: bit 5, (ix+d/iy+d)
        &z80::bit<0x20U, mode>, // $6E: bit 5, (ix+d/iy+d)
        &z80::bit<0x20U, mode>, // $6F: bit 5, (ix+d/iy+d)
        &z80::bit<0x40U, mode>, // $70: bit 6, (ix+d/iy+d)
        &z80::bit<0x40U, mode>, // $71: bit 6, (ix+d/iy+d)
        &z80::bit<0x40U, mode>, // $72: bit 6, (ix+d/iy+d)
        &z80::bit<0x40U, mode>, // $73: bit 6, (ix+d/iy+d)
        &z80::bit<0x40U, mode>, // $74: bit 6, (ix+d/iy+d)
        &z80::bit<0x40U, mode>, // $75: bit 6, (ix+d/iy+d)
        &z80::bit<0x40U, mode>, // $76: bit 6, (ix+d/iy+d)
        &z80::bit<0x40U, mode>, // $77: bit 6, (ix+d/iy+d)
        &z80::bit<0x80U, mode>, // $78: bit 7, (ix+d/iy+d)
        &z80::bit<0x80U, mode>, // $79: bit 7, (ix+d/iy+d)
        &z80::bit<0x80U, mode>, // $7A: bit 7, (ix+d/iy+d)
        &z80::bit<0x80U, mode>, // $7B: bit 7, (ix+d/iy+d)
        &z80::bit<0x80U, mode>, // $7C: bit 7, (ix+d/iy+d)
        &z80::bit<0x80U, mode>, // $7D: bit 7, (ix+d/iy+d)
        &z80::bit<0x80U, mode>, // $7E: bit 7, (ix+d/iy+d)
        &z80::bit<0x80U, mode>, // $7F: bit 7, (ix+d/iy+d)
        &z80::res<0xFEU, mode, AddressMode::RegisterB>, // $80: res 0, (ix+d/iy+d), b
        &z80::res<0xFEU, mode, AddressMode::RegisterC>, // $81: res 0, (ix+d/iy+d), c
        &z80::res<0xFEU, mode, AddressMode::RegisterD>, // $82: res 0, (ix+d/iy+d), d
        &z80::res<0xFEU, mode, AddressMode::RegisterE>, // $83: res 0, (ix+d/iy+d), e
        &z80::res<0xFEU, mode, AddressMode::RegisterH>, // $84: res 0, (ix+d/iy+d), h
        &z80::res<0xFEU, mode, AddressMode::RegisterL>, // $85: res 0, (ix+d/iy+d), l
        &z80::res<0xFEU, mode, mode>, // $86: res 0, (ix+d/iy+d)
        &z80::res<0xFEU, mode, AddressMode::Accumulator>, // $87: res 0, (ix+d/iy+d), a
        &z80::res<0xFDU, mode, AddressMode::RegisterB>, // $88: res 1, (ix+d/iy+d), b
        &z80::res<0xFDU, mode, AddressMode::RegisterC>, // $89: res 1, (ix+d/iy+d), c
        &z80::res<0xFDU, mode, AddressMode::RegisterD>, // $8A: res 1, (ix+d/iy+d), d
        &z80::res<0xFDU, mode, AddressMode::RegisterE>, // $8B: res 1, (ix+d/iy+d), e
        &z80::res<0xFDU, mode, AddressMode::RegisterH>, // $8C: res 1, (ix+d/iy+d), h
        &z80::res<0xFDU, mode, AddressMode::RegisterL>, // $8D: res 1, (ix+d/iy+d), l
        &z80::res<0xFDU, mode, mode>, // $8E: res 1, (ix+d/iy+d)
        &z80::res<0xFDU, mode, AddressMode::Accumulator>, // $8F: res 1, (ix+d/iy+d), a
        &z80::res<0xFBU, mode, AddressMode::RegisterB>, // $90: res 2, (ix+d/iy+d), b
        &z80::res<0xFBU, mode, AddressMode::RegisterC>, // $91: res 2, (ix+d/iy+d), c
        &z80::res<0xFBU, mode, AddressMode::RegisterD>, // $92: res 2, (ix+d/iy+d), d
        &z80::res<0xFBU, mode, AddressMode::RegisterE>, // $93: res 2, (ix+d/iy+d), e
        &z80::res<0xFBU, mode, AddressMode::RegisterH>, // $94: res 2, (ix+d/iy+d), h
        &z80::res<0xFBU, mode, AddressMode::RegisterL>, // $95: res 2, (ix+d/iy+d), l
        &z80::res<0xFBU, mode, mode>, // $96: res 2, (ix+d/iy+d)
        &z80::res<0xFBU, mode, AddressMode::Accumulator>, // $97: res 2, (ix+d/iy+d), a
        &z80::res<0xF7U, mode, AddressMode::RegisterB>, // $98: res 3, (ix+d/iy+d), b
        &z80::res<0xF7U, mode, AddressMode::RegisterC>, // $99: res 3, (ix+d/iy+d), c
        &z80::res<0xF7U, mode, AddressMode::RegisterD>, // $9A: res 3, (ix+d/iy+d), d
        &z80::res<0xF7U, mode, AddressMode::RegisterE>, // $9B: res 3, (ix+d/iy+d), e
        &z80::res<0xF7U, mode, AddressMode::RegisterH>, // $9C: res 3, (ix+d/iy+d), h
        &z80::res<0xF7U, mode, AddressMode::RegisterL>, // $9D: res 3, (ix+d/iy+d), l
        &z80::res<0xF7U, mode, mode>, // $9E: res 3, (ix+d/iy+d)
        &z80::res<0xF7U, mode, AddressMode::Accumulator>, // $9F: res 3, (ix+d/iy+d), a
        &z80::res<0xEFU, mode, AddressMode::RegisterB>, // $A0: res 4, (ix+d/iy+d), b
        &z80::res<0xEFU, mode, AddressMode::RegisterC>, // $A1: res 4, (ix+d/iy+d), c
        &z80::res<0xEFU, mode, AddressMode::RegisterD>, // $A2: res 4, (ix+d/iy+d), d
        &z80::res<0xEFU, mode, AddressMode::RegisterE>, // $A3: res 4, (ix+d/iy+d), e
        &z80::res<0xEFU, mode, AddressMode::RegisterH>, // $A4: res 4, (ix+d/iy+d), h
        &z80::res<0xEFU, mode, AddressMode::RegisterL>, // $A5: res 4, (ix+d/iy+d), l
        &z80::res<0xEFU, mode, mode>, // $A6: res 4, (ix+d/iy+d)
        &z80::res<0xEFU, mode, AddressMode::Accumulator>, // $A7: res 4, (ix+d/iy+d), a
        &z80::res<0xDFU, mode, AddressMode::RegisterB>, // $A8: res 5, (ix+d/iy+d), b
        &z80::res<0xDFU, mode, AddressMode::RegisterC>, // $A9: res 5, (ix+d/iy+d), c
        &z80::res<0xDFU, mode, AddressMode::RegisterD>, // $AA: res 5, (ix+d/iy+d), d
        &z80::res<0xDFU, mode, AddressMode::RegisterE>, // $AB: res 5, (ix+d/iy+d), e
        &z80::res<0xDFU, mode, AddressMode::RegisterH>, // $AC: res 5, (ix+d/iy+d), h
        &z80::res<0xDFU, mode, AddressMode::RegisterL>, // $AD: res 5, (ix+d/iy+d), l
        &z80::res<0xDFU, mode, mode>, // $AE: res 5, (ix+d/iy+d)
        &z80::res<0xDFU, mode, AddressMode::Accumulator>, // $AF: res 5, (ix+d/iy+d), a
        &z80::res<0xBFU, mode, AddressMode::RegisterB>, // $B0: res 6, (ix+d/iy+d), b
        &z80::res<0xBFU, mode, AddressMode::RegisterC>, // $B1: res 6, (ix+d/iy+d), c
        &z80::res<0xBFU, mode, AddressMode::RegisterD>, // $B2: res 6, (ix+d/iy+d), d
        &z80::res<0xBFU, mode, AddressMode::RegisterE>, // $B3: res 6, (ix+d/iy+d), e
        &z80::res<0xBFU, mode, AddressMode::RegisterH>, // $B4: res 6, (ix+d/iy+d), h
        &z80::res<0xBFU, mode, AddressMode::RegisterL>, // $B5: res 6, (ix+d/iy+d), l
        &z80::res<0xBFU, mode, mode>, // $B6: res 6, (ix+d/iy+d)
        &z80::res<0xBFU, mode, AddressMode::Accumulator>, // $B7: res 6, (ix+d/iy+d), a
        &z80::res<0x7FU, mode, AddressMode::RegisterB>, // $B8: res 7, (ix+d/iy+d), b
        &z80::res<0x7FU, mode, AddressMode::RegisterC>, // $B9: res 7, (ix+d/iy+d), c
        &z80::res<0x7FU, mode, AddressMode::RegisterD>, // $BA: res 7, (ix+d/iy+d), d
        &z80::res<0x7FU, mode, AddressMode::RegisterE>, // $BB: res 7, (ix+d/iy+d), e
        &z80::res<0x7FU, mode, AddressMode::RegisterH>, // $BC: res 7, (ix+d/iy+d), h
        &z80::res<0x7FU, mode, AddressMode::RegisterL>, // $BD: res 7, (ix+d/iy+d), l
        &z80::res<0x7FU, mode, mode>, // $BE: res 7, (ix+d/iy+d)
        &z80::res<0x7FU, mode, AddressMode::Accumulator>, // $BF: res 7, (ix+d/iy+d), a
        &z80::set<0x01U, mode, AddressMode::RegisterB>, // $C0: set 0, (ix+d/iy+d), b
        &z80::set<0x01U, mode, AddressMode::RegisterC>, // $C1: set 0, (ix+d/iy+d), c
        &z80::set<0x01U, mode, AddressMode::RegisterD>, // $C2: set 0, (ix+d/iy+d), d
        &z80::set<0x01U, mode, AddressMode::RegisterE>, // $C3: set 0, (ix+d/iy+d), e
        &z80::set<0x01U, mode, AddressMode::RegisterH>, // $C4: set 0, (ix+d/iy+d), h
        &z80::set<0x01U, mode, AddressMode::RegisterL>, // $C5: set 0, (ix+d/iy+d), l
        &z80::set<0x01U, mode, mode>, // $C6: set 0, (ix+d/iy+d)
        &z80::set<0x01U, mode, AddressMode::Accumulator>, // $C7: set 0, (ix+d/iy+d), a
        &z80::set<0x02U, mode, AddressMode::RegisterB>, // $C8: set 1, (ix+d/iy+d), b
        &z80::set<0x02U, mode, AddressMode::RegisterC>, // $C9: set 1, (ix+d/iy+d), c
        &z80::set<0x02U, mode, AddressMode::RegisterD>, // $CA: set 1, (ix+d/iy+d), d
        &z80::set<0x02U, mode, AddressMode::RegisterE>, // $CB: set 1, (ix+d/iy+d), e
        &z80::set<0x02U, mode, AddressMode::RegisterH>, // $CC: set 1, (ix+d/iy+d), h
        &z80::set<0x02U, mode, AddressMode::RegisterL>, // $CD: set 1, (ix+d/iy+d), l
        &z80::set<0x02U, mode, mode>, // $CE: set 1, (ix+d/iy+d)
        &z80::set<0x02U, mode, AddressMode::Accumulator>, // $CF: set 1, (ix+d/iy+d), a
        &z80::set<0x04U, mode, AddressMode::RegisterB>, // $D0: set 2, (ix+d/iy+d), b
        &z80::set<0x04U, mode, AddressMode::RegisterC>, // $D1: set 2, (ix+d/iy+d), c
        &z80::set<0x04U, mode, AddressMode::RegisterD>, // $D2: set 2, (ix+d/iy+d), d
        &z80::set<0x04U, mode, AddressMode::RegisterE>, // $D3: set 2, (ix+d/iy+d), e
        &z80::set<0x04U, mode, AddressMode::RegisterH>, // $D4: set 2, (ix+d/iy+d), h
        &z80::set<0x04U, mode, AddressMode::RegisterL>, // $D5: set 2, (ix+d/iy+d), l
        &z80::set<0x04U, mode, mode>, // $D6: set 2, (ix+d/iy+d)
        &z80::set<0x04U, mode, AddressMode::Accumulator>, // $D7: set 2, (ix+d/iy+d), a
        &z80::set<0x08U, mode, AddressMode::RegisterB>, // $D8: set 3, (ix+d/iy+d), b
        &z80::set<0x08U, mode, AddressMode::RegisterC>, // $D9: set 3, (ix+d/iy+d), c
        &z80::set<0x08U, mode, AddressMode::RegisterD>, // $DA: set 3, (ix+d/iy+d), d
        &z80::set<0x08U, mode, AddressMode::RegisterE>, // $DB: set 3, (ix+d/iy+d), e
        &z80::set<0x08U, mode, AddressMode::RegisterH>, // $DC: set 3, (ix+d/iy+d), h
        &z80::set<0x08U, mode, AddressMode::RegisterL>, // $DD: set 3, (ix+d/iy+d), l
        &z80::set<0x08U, mode, mode>, // $DE: set 3, (ix+d/iy+d)
        &z80::set<0x08U, mode, AddressMode::Accumulator>, // $DF: set 3, (ix+d/iy+d), a
        &z80::set<0x10U, mode, AddressMode::RegisterB>, // $E0: set 4, (ix+d/iy+d), b
        &z80::set<0x10U, mode, AddressMode::RegisterC>, // $E1: set 4, (ix+d/iy+d), c
        &z80::set<0x10U, mode, AddressMode::RegisterD>, // $E2: set 4, (ix+d/iy+d), d
        &z80::set<0x10U, mode, AddressMode::RegisterE>, // $E3: set 4, (ix+d/iy+d), e
        &z80::set<0x10U, mode, AddressMode::RegisterH>, // $E4: set 4, (ix+d/iy+d), h
        &z80::set<0x10U, mode, AddressMode::RegisterL>, // $E5: set 4, (ix+d/iy+d), l
        &z80::set<0x10U, mode, mode>, // $E6: set 4, (ix+d/iy+d)
        &z80::set<0x10U, mode, AddressMode::Accumulator>, // $E7: set 4, (ix+d/iy+d), a
        &z80::set<0x20U, mode, AddressMode::RegisterB>, // $E8: set 5, (ix+d/iy+d), b
        &z80::set<0x20U, mode, AddressMode::RegisterC>, // $E9: set 5, (ix+d/iy+d), c
        &z80::set<0x20U, mode, AddressMode::RegisterD>, // $EA: set 5, (ix+d/iy+d), d
        &z80::set<0x20U, mode, AddressMode::RegisterE>, // $EB: set 5, (ix+d/iy+d), e
        &z80::set<0x20U, mode, AddressMode::RegisterH>, // $EC: set 5, (ix+d/iy+d), h
        &z80::set<0x20U, mode, AddressMode::RegisterL>, // $ED: set 5, (ix+d/iy+d), l
        &z80::set<0x20U, mode, mode>, // $EE: set 5, (ix+d/iy+d)
        &z80::set<0x20U, mode, AddressMode::Accumulator>, // $EF: set 5, (ix+d/iy+d), a
        &z80::set<0x40U, mode, AddressMode::RegisterB>, // $F0: set 6, (ix+d/iy+d), b
        &z80::set<0x40U, mode, AddressMode::RegisterC>, // $F1: set 6, (ix+d/iy+d), c
        &z80::set<0x40U, mode, AddressMode::RegisterD>, // $F2: set 6, (ix+d/iy+d), d
        &z80::set<0x40U, mode, AddressMode::RegisterE>, // $F3: set 6, (ix+d/iy+d), e
        &z80::set<0x40U, mode, AddressMode::RegisterH>, // $F4: set 6, (ix+d/iy+d), h
        &z80::set<0x40U, mode, AddressMode::RegisterL>, // $F5: set 6, (ix+d/iy+d), l
        &z80::set<0x40U, mode, mode>, // $F6: set 6, (ix+d/iy+d)
        &z80::set<0x40U, mode, AddressMode::Accumulator>, // $F7: set 6, (ix+d/iy+d), a
        &z80::set<0x80U, mode, AddressMode::RegisterB>, // $F8: set 7, (ix+d/iy+d), b
        &z80::set<0x80U, mode, AddressMode::RegisterC>, // $F9: set 7, (ix+d/iy+d), c
        &z80::set<0x80U, mode, AddressMode::RegisterD>, // $FA: set 7, (ix+d/iy+d), d
        &z80::set<0x80U, mode, AddressMode::RegisterE>, // $FB: set 7, (ix+d/iy+d), e
        &z80::set<0x80U, mode, AddressMode::RegisterH>, // $FC: set 7, (ix+d/iy+d), h
        &z80::set<0x80U, mode, AddressMode::RegisterL>, // $FD: set 7, (ix+d/iy+d), l
        &z80::set<0x80U, mode, mode>, // $FE: set 7, (ix+d/iy+d)
        &z80::set<0x80U, mode, AddressMode::Accumulator>, // $FF: set 7, (ix+d/iy+d), a
    };

    // interrupt stuff
    bool nmiRequested {false}, intRequested {false};
    u8 ivector {0};

    // shadow registers
    u8 a2 {0xFFU};
    Registers regs2 {};
    Flags flags2 {};
};

template<typename Memory>
int z80<Memory>::run(const int cycles)
{
    requested = cycles;
    while (requested > 0) step();
    return requested;
}

template<typename Memory>
void z80<Memory>::step()
{
    incrementRefresh();

    // handle non maskable interrupts
    if (nmiRequested) {
        iff2 = iff1;
        iff1 = halted = nmiRequested = false;
        rst<0x66>();
        requested -= 11;
        return;
    }

    // handle maskable interrupts
    if (intRequested & iff1) {
        iff1 = iff2 = halted = intRequested = false;
        pushpc();

        if (imode == 0) {
            pc = ivector;
            requested -= 11;
        } else if (imode == 1) {
            pc = 0x38;
            requested -= 13;
        } else if (imode == 2) {
            pc = (i << 8U) | (ivector & 0xFE);
            requested -= 19;
        }
        return;
    }

    // else execute an instruction
    if (!halted)
        tick();
    else
        requested -= 4; // nop while in halted state
}

template<typename Memory>
void z80<Memory>::reset()
{
    pc = i = r = ix = iy = imode = ivector = 0;
    regs.b = regs.c = regs.d = regs.e = regs.h = regs.l = 0;
    regs2.b = regs2.c = regs2.d = regs2.e = regs2.h = regs2.l = 0;
    sp = 0xFFFF;
    a = a2 = 0xFF;
    iff1 = iff2 = halted = false;
    setf(flags, 0xFFU);
    setf(flags2, 0xFFU);
}

template<typename Memory>
void z80<Memory>::reqNmi()
{
    nmiRequested = true;
}

template<typename Memory>
void z80<Memory>::reqInt(const u8 vector)
{
    intRequested = true;
    ivector = vector;
}

template<typename Memory>
std::string z80<Memory>::toString(const unsigned long long cycle) const
{
    return std::format(
        "PC: {:0>4X}, AF: {:0>4X}, BC: {:0>4X}, DE: {:0>4X}, HL: {:0>4X}, SP: {:0>4X}, IX: {:0>4X}, IY: {:0>4X}, I: {:0>2X}, R: {:0>2X}\t({:0>2X} {:0>2X} {:0>2X} {:0>2X}), cyc: {:d}",
        pc,
        pair(a, getf()),
        pair(regs.b, regs.c),
        pair(regs.d, regs.e),
        pair(regs.h, regs.l),
        sp, ix, iy, i, r,
        memory.ram[pc],
        memory.ram[pc + 1U],
        memory.ram[pc + 2U],
        memory.ram[pc + 3U],
        cycle);
}

template<typename Memory>
void z80<Memory>::setf(Flags& f, const u8 val)
{
    flags.sf = val & sbit;
    flags.zf = val & zbit;
    flags.yf = val & ybit;
    flags.hf = val & hbit;
    flags.xf = val & xbit;
    flags.pf = val & pbit;
    flags.nf = val & nbit;
    flags.cf = val & cbit;
}

#endif //Z80_LIBRARY_H
