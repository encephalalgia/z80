#ifndef Z80_LIBRARY_H
#define Z80_LIBRARY_H

#include <cstdint>
#include <algorithm>
#include <bit>
#include <array>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using s8 = std::int8_t;

/**
 * \brief
 * \tparam Memory
 * \tparam IO
 */
template<typename Memory, typename IO>
class z80 {
public:
    /**
     * \brief
     * \param cycles
     * \return
     */
    int run(int cycles);
    void interrupt();

    /**
     * \brief
     * \param addr
     * \param val
     */
    void write(u16 addr, u8 val) { memory.write(addr, val); }

    /**
     * \brief
     * \param addr
     * \param val
     */
    void write(u16 addr, u16 val) { memory.write(addr, val); }

    /**
     * \brief
     * \param addr
     * \return
     */
    u8 read8(u16 addr) { return memory.read8(addr); }

    /**
     * \brief
     * \param addr
     * \return
     */
    u16 read16(u16 addr) { return memory.read16(addr); }

    /**
     * \brief
     * \param addr
     * \return
     */
    u8 input(u8 addr) { return io.input(addr); }

    /**
     * \brief
     * \param addr
     * \param val
     */
    void output(u8 addr, u8 val) { io.output(addr, val); }

    /**
     * \brief Flags are stored separetly, to get them as a single status byte they must be or'd together.
     * \return The flag register as a single byte.
     */
    [[nodiscard]] u8 flag_() const { return flags.sf | flags.zf | flags.yf | flags.hf | flags.xf | flags.pf | flags.nf | flags.cf; }

    // internals
    struct Registers {
        u8 b {}, c {}, d {}, e {}, h {}, l {};
    };
    struct Flags {
        u8 sf {}, zf {}, yf {}, hf {}, xf {}, pf {}, nf {}, cf {};
    };

    Registers regs {};
    Flags flags {};
    u16 pc {}, sp {}, ix {}, iy {};
    u8 i {}, r {}, a {};
private:
    static constexpr u8 sbit {0b10000000}; // bit 7: sign flag
    static constexpr u8 zbit {0b01000000}; // bit 6: zero flag
    static constexpr u8 ybit {0b00100000}; // bit 5: undocumented flag (copy of bit 5 of the result)
    static constexpr u8 hbit {0b00010000}; // bit 4: half-carry flag
    static constexpr u8 xbit {0b00001000}; // bit 3: undocumented flag (copy of bit 3 of the result)
    static constexpr u8 pbit {0b00000100}; // bit 2: parity flag
    static constexpr u8 nbit {0b00000010}; // bit 1: add/subtract flag
    static constexpr u8 cbit {0b00000001}; // bit 0: carry flag

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
            case AddressMode::Extended: write(fetch16(), val); return;
            case AddressMode::IndexedIX: write(fetch8() + ix, val); return;
            case AddressMode::IndexedIY: write(fetch8() + iy, val); return;
            case AddressMode::Interrupt: i = val; return;
            case AddressMode::Refresh: r = val; return;
            case AddressMode::Accumulator: a = val; return;
            case AddressMode::RegisterB: regs.b = val; return;
            case AddressMode::RegisterC: regs.c = val; return;
            case AddressMode::RegisterD: regs.d = val; return;
            case AddressMode::RegisterE: regs.e = val; return;
            case AddressMode::RegisterH: regs.h = val; return;
            case AddressMode::RegisterL: regs.l = val; return;
            case AddressMode::RegisterBC: regs.b = val >> 8; regs.c = val & 0xFF; return;
            case AddressMode::RegisterDE: regs.d = val >> 8; regs.e = val & 0xFF; return;
            case AddressMode::RegisterHL: regs.h = val >> 8; regs.l = val & 0xFF; return;
            case AddressMode::RegisterSP: sp = val; return;
            case AddressMode::RegisterAF: {
                a = val >> 8U;
                flags.sf = val & sbit;
                flags.zf = val & zbit;
                flags.yf = val & ybit;
                flags.hf = val & hbit;
                flags.xf = val & xbit;
                flags.pf = val & pbit;
                flags.nf = val & nbit;
                flags.cf = val & cbit;
                return;
            }
            case AddressMode::RegisterIX: ix = val; return;
            case AddressMode::RegisterIXH: ix &= ~0xFF00 | val << 8U;
            case AddressMode::RegisterIXL: ix &= ~0xFF | val & 0xFF;
            case AddressMode::RegisterIY: iy = val; return;
            case AddressMode::RegisterIYH: iy &= ~0xFF00 | val << 8U;
            case AddressMode::RegisterIYL: iy &= ~0xFF | val & 0xFF;
            case AddressMode::RegisterIndirectBC: write(regs.b << 8 | regs.c, val); return;
            case AddressMode::RegisterIndirectDE: write(regs.d << 8 | regs.e, val); return;
            case AddressMode::RegisterIndirectHL: write(regs.h << 8 | regs.l, val); return;
            case AddressMode::RegisterIndirectSP: write(sp, val); return;
        }
    }

    template<AddressMode mode>
    u16 getOperand()
    {
        switch (mode) {
            case AddressMode::Immediate: return fetch8();
            case AddressMode::ImmediateEx: return fetch16();
            case AddressMode::Extended: return read8(fetch16());
            case AddressMode::IndexedIX: return read8(fetch8() + ix);
            case AddressMode::IndexedIY: return read8(fetch8() + iy);
            case AddressMode::Interrupt: return i;
            case AddressMode::Refresh: return r;
            case AddressMode::Accumulator: return a;
            case AddressMode::RegisterB: return regs.b;
            case AddressMode::RegisterC: return regs.c;
            case AddressMode::RegisterD: return regs.d;
            case AddressMode::RegisterE: return regs.e;
            case AddressMode::RegisterH: return regs.h;
            case AddressMode::RegisterL: return regs.l;
            case AddressMode::RegisterBC: return regs.b << 8 | regs.c;
            case AddressMode::RegisterDE: return regs.d << 8 | regs.e;
            case AddressMode::RegisterHL: return regs.h << 8 | regs.l;
            case AddressMode::RegisterSP: return sp;
            case AddressMode::RegisterAF: return a << 8U | flag_();
            case AddressMode::RegisterIX: return ix;
            case AddressMode::RegisterIXH: return ix >> 8U;
            case AddressMode::RegisterIXL: return ix & 0xFF;
            case AddressMode::RegisterIY: return iy;
            case AddressMode::RegisterIYH: return iy >> 8U;
            case AddressMode::RegisterIYL: return iy & 0xFF;
            case AddressMode::RegisterIndirectBC: return read8(regs.b << 8 | regs.c);
            case AddressMode::RegisterIndirectDE: return read8(regs.d << 8 | regs.e);
            case AddressMode::RegisterIndirectHL: return read8(regs.h << 8 | regs.l);
            case AddressMode::RegisterIndirectSP: return read8(sp);
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

    /**
     * \brief Internal function used to execute a single instruction (macro op).
     */
    void tick();

    // Helper member functions
    [[nodiscard]] static bool carry(const u16 sum) { return sum > 0xFFU; }
    [[nodiscard]] static bool carry(const u32 sum) { return sum > 0xFFFFU; }
    [[nodiscard]] static bool halfcy(const u8 addend, const u8 augend, const u16 sum) { return (addend ^ augend ^ sum) & 0x10U; }
    [[nodiscard]] static bool halfcy(const u16 addend, const u16 augend, const u32 sum) { return (addend ^ augend ^ sum) & 0x100U; }
    [[nodiscard]] static bool underflow(const u8 addend, const u8 augend, const u16 sum) { return ((addend ^ sum) & (augend ^ sum) & 0x80U) != 0; }
    [[nodiscard]] static bool overflow(const u8 minuend, const u8 subtrahend, const u16 difference) { return ((difference ^ minuend) & (subtrahend ^ minuend) & 0x80U) != 0; }
    [[nodiscard]] static bool borrow(const u8 minuend, const u8 subtrahend) { return minuend < subtrahend; }
    [[nodiscard]] static bool halfbw(const u8 minuend, const u8 subtrahend, const u16 difference) { return ~(minuend ^ subtrahend ^ difference) & 0x10U; }
    [[nodiscard]] u8 fetch8() { return read8(pc++); }
    [[nodiscard]] u16 fetch16() { pc += 2; return read16(pc - 2); };
    [[nodiscard]] static u16 pair(const u8 hi, const u8 lo) { return hi << 8U | lo; }
    [[nodiscard]] static bool parity(const u8 val) { return std::popcount(val) % 2 == 0; }

    u16 top()
    {
        u16 top {read8(sp++)};
        top |= read8(sp++) << 8U;
        return top;
    }

    void pushpc()
    {
        write(sp--, static_cast<u8>(++pc >> 8U));
        write(sp--, static_cast<u8>(pc & 0xFFU));
    }

    void commonFlags(const u8 val)
    {
        flags.sf = val & sbit;
        flags.zf = val == 0 ? zbit : 0;
        flags.yf = val & ybit;
        flags.xf = val & xbit;
    }

    void logicFlags()
    {
        flags.cf = 0;
        flags.nf = 0;
        flags.hf = 0;
        flags.pf = parity(a) ? pbit : 0;
        commonFlags(a);
    }

    void shiftFlags1(const u8 val)
    {
        flags.pf = parity(val) ? pbit : 0;
        flags.hf = 0;
        flags.nf = 0;
        commonFlags(val);
    }

    void shiftFlags2(const u8 val)
    {
        flags.hf = 0;
        flags.nf = 0;
        flags.yf = val & ybit;
        flags.xf = val & xbit;
    }

    void blockTransferFlags(const u8 val, const u8 operand)
    {
        flags.nf = 0;
        flags.pf = pair(regs.b, regs.c) != 0 ? pbit : 0;
        flags.yf = val + operand & 0b1000;
        flags.xf = val + operand & 0b10;
    }

    void blockIOFlags(const u8 n)
    {
        --regs.b;
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) + n);
        flags.nf = nbit;
        flags.zf = regs.b == 0 ? zbit : 0;
    }

    void addition(const u8 augend, const u16 sum)
    {
        flags.cf = carry(sum) ? cbit : 0;
        flags.hf = halfcy(a, augend, sum) ? hbit : 0;
        flags.pf = underflow(a, augend, sum) != 0 ? pbit : 0;
        flags.nf = 0;
        a = static_cast<u8>(sum);
        commonFlags(a);
    }

    void subtraction(const u8 subtrahend, const u16 difference)
    {
        flags.hf = halfbw(a, subtrahend, difference) ? hbit : 0;
        flags.pf = overflow(a, subtrahend, difference) ? pbit : 0;
        flags.nf = nbit;
        a = static_cast<u8>(difference);
        commonFlags(a);
    }

    // Load and Exchange
    template<AddressMode mode1, AddressMode mode2>
    void ld() { setOperand<mode1>(getOperand<mode2>()); }

    template<AddressMode mode>
    void push()
    {
        const u16 operand {getOperand<mode>()};
        write(sp--, static_cast<u8>(operand >> 8U));
        write(sp--, static_cast<u8>(operand & 0xFFU));
    }

    template<AddressMode mode>
    void pop() { setOperand<mode>(top()); }

    void exaf()
    {
        std::swap(a, a2);
        std::swap(flags, flags2);
    }

    template<AddressMode mode>
    void exsp()
    {
        const u16 temp {getOperand<mode>()};
        setOperand<mode>(read8(sp + 1) << 8U | read8(sp));
        write(sp + 1, static_cast<u8>(temp >> 8U));
        write(sp, static_cast<u8>(temp & 0xFF));
    }

    void exde()
    {
        std::swap(regs.d, regs.h);
        std::swap(regs.e, regs.l);
    }

    void exx() { std::swap(regs, regs2); }

    // Block Transfer and Search
    void ldi()
    {
        const u8 tmp {read8(pair(regs.h, regs.l))};
        write(pair(regs.d, regs.e), tmp);
        setOperand<AddressMode::RegisterBC>(pair(regs.b, regs.c) - 1);
        setOperand<AddressMode::RegisterDE>(pair(regs.d, regs.e) + 1);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) + 1);
        blockTransferFlags(tmp, a);
    }

    void ldir()
    {
        ldi();
        if (flags.pf)
            pc -= 2;
    }

    void ldd()
    {
        const u8 tmp {read8(pair(regs.h, regs.l))};
        write(pair(regs.d, regs.e), tmp);
        setOperand<AddressMode::RegisterBC>(pair(regs.b, regs.c) - 1);
        setOperand<AddressMode::RegisterDE>(pair(regs.d, regs.e) - 1);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) - 1);
        blockTransferFlags(tmp, a);
    }

    void lddr()
    {
        ldd();
        if (flags.pf)
            pc -= 2;
    }

    void cpi()
    {
        const u8 subtrahend {read8(pair(regs.h, regs.l))};
        const u16 difference {static_cast<u16>(a - subtrahend)};
        flags.cf = borrow(a, subtrahend) ? cbit : 0;
        flags.hf = halfbw(a, subtrahend, difference) ? hbit : 0;
        const u8 tmp {static_cast<u8>(difference)};
        setOperand<AddressMode::RegisterBC>(pair(regs.b, regs.c) - 1);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) + 1);
        blockTransferFlags(tmp, -(flags.hf >> 4));
    }

    void cpir()
    {
        cpi();
        if (flags.pf and !flags.zf)
            pc -= 2;
    }

    void cpd()
    {
        const u8 subtrahend {read8(pair(regs.h, regs.l))};
        const u16 difference {static_cast<u16>(a - subtrahend)};
        flags.cf = borrow(a, subtrahend) ? cbit : 0;
        flags.hf = halfbw(a, subtrahend, difference) ? hbit : 0;
        const u8 tmp {static_cast<u8>(difference)};
        setOperand<AddressMode::RegisterBC>(pair(regs.b, regs.c) - 1);
        setOperand<AddressMode::RegisterHL>(pair(regs.h, regs.l) - 1);
        blockTransferFlags(tmp, -(flags.hf >> 4));
    }

    void cpdr()
    {
        cpd();
        if (flags.pf and !flags.zf)
            pc -= 2;
    }

    // Arithmetic and Logical
    template<AddressMode mode>
    void add8()
    {
        const u8 augend {static_cast<u8>(getOperand<mode>())};
        const u16 sum {static_cast<u16>(a + augend)};
        addition(augend, sum);
    }

    template<AddressMode mode>
    void adc8()
    {
        const u8 augend {static_cast<u8>(getOperand<mode>())};
        const u16 sum {static_cast<u16>(a + augend + flags.cf)};
        addition(augend, sum);
    }

    template<AddressMode mode>
    void sub()
    {
        const u8 subtrahend {static_cast<u8>(getOperand<mode>())};
        const u16 difference {static_cast<u16>(a - subtrahend)};
        flags.cf = borrow(a, subtrahend) ? cbit : 0;
        subtraction(subtrahend, difference);
    }

    template<AddressMode mode>
    void sbc8()
    {
        const u8 subtrahend {static_cast<u8>(getOperand<mode>())};
        const u16 difference {static_cast<u16>(a - subtrahend - flags.cf)};
        flags.cf = borrow(a, subtrahend + flags.cf) ? cbit : 0;
        subtraction(subtrahend, difference);
    }

    template<AddressMode mode>
    void inc8()
    {
        const u8 operand {static_cast<u8>(getOperand<mode>())};
        const u8 res {static_cast<u8>(operand + 1U)};
        flags.hf = (operand & 0xFU) == 0; // only case for half carry is 0b1111 + 0b1 = 0b10000;
        flags.pf = underflow(operand, 1U, res) != 0 ? pbit : 0;
        flags.nf = 0;
        commonFlags(res);
        setOperand<mode>(res);
    }

    template<AddressMode mode>
    void dec8()
    {
        u8 operand {static_cast<u8>(getOperand<mode>())};
        const u8 res {--operand};
        flags.hf = (operand & 0xFU) != 0xF; // only case for half borrow is 0b10000 - 0b1 = 0b1111
        flags.pf = overflow(operand, 1U, res) != 0 ? pbit : 0;
        flags.nf = nbit;
        commonFlags(res);
        setOperand<mode>(res);
    }

    template<AddressMode mode>
    void land()
    {
        a &= getOperand<mode>();
        logicFlags();
    }

    template<AddressMode mode>
    void lxor()
    {
        a ^= getOperand<mode>();
        logicFlags();
    }

    template<AddressMode mode>
    void lor()
    {
        a |= getOperand<mode>();
        logicFlags();
    }

    template<AddressMode mode>
    void cp()
    {
        const u8 subtrahend {static_cast<u8>(getOperand<mode>())};
        const u16 difference {static_cast<u16>(a - subtrahend)};
        flags.cf = borrow(a, subtrahend) ? cbit : 0;
        flags.hf = halfbw(a, subtrahend, difference) ? hbit : 0;
        flags.pf = overflow(a, subtrahend, difference) ? pbit : 0;
        flags.nf = nbit;
        flags.yf = subtrahend & ybit;
        flags.xf = subtrahend & xbit;
        flags.zf = difference == 0 ? zbit : 0;
        flags.sf = static_cast<u8>(difference) & sbit;
    }

    template<AddressMode mode1, AddressMode mode2>
    void add16()
    {
        const u16 addend {getOperand<mode1>()};
        const u16 augend {getOperand<mode2>()};
        const u32 sum {static_cast<u32>(addend + augend)};
        flags.cf = carry(sum) ? cbit : 0;
        flags.hf = halfcy(addend, augend, sum) ? hbit : 0;
        flags.nf = 0;
        flags.yf = sum & ybit;
        flags.xf = sum & xbit;
        setOperand<mode1>(static_cast<u16>(sum));
    }

    template<AddressMode mode1, AddressMode mode2>
    void adc16()
    {
        const u16 addend {getOperand<mode1>()};
        const u16 augend {getOperand<mode2>()};
        const u32 sum {static_cast<u32>(addend + augend + flags.cf)};
        flags.cf = carry(sum) ? cbit : 0;
        flags.hf = halfcy(addend, augend, sum) ? hbit : 0;
        flags.pf = underflow(addend, augend, sum) != 0 ? pbit : 0;
        flags.nf = 0;
        flags.yf = sum & ybit;
        flags.xf = sum & xbit;
        flags.zf = sum == 0 ? zbit : 0;
        flags.sf = static_cast<u16>(sum) & sbit;
        setOperand<mode1>(static_cast<u16>(sum));
    }

    template<AddressMode mode1, AddressMode mode2>
    void sbc16()
    {
        const u16 minuend {getOperand<mode1>()};
        const u16 subtrahend {getOperand<mode2>()};
        const u32 difference {static_cast<u16>(minuend - subtrahend - flags.cf)};
        flags.cf = borrow(minuend, subtrahend + flags.cf) ? cbit : 0;
        flags.hf = halfbw(minuend, subtrahend, difference) ? hbit : 0;
        flags.pf = overflow(minuend, subtrahend, difference) ? pbit : 0;
        flags.nf = nbit;
        flags.yf = difference & ybit;
        flags.xf = difference & xbit;
        flags.zf = difference == 0 ? zbit : 0;
        flags.sf = static_cast<u16>(difference) & sbit;
        setOperand<mode1>(static_cast<u16>(difference));
    }

    template<AddressMode mode>
    void inc16() { setOperand<mode>(getOperand<mode>() + 1); }

    template<AddressMode mode>
    void dec16() { setOperand<mode>(getOperand<mode>() + 1); }

    void cpl()
    {
        a = ~a;
        flags.nf = nbit;
        flags.hf = hbit;
    }

    void neg()
    {
        const u8 res {static_cast<u8>(0 - a)};
        flags.cf = a != 0 ? cbit : 0;
        flags.pf = a == 0x80U ? pbit : 0;
        flags.hf = halfbw(a, 0, res) ? hbit : 0;
        flags.nf = nbit;
        a = res;
        commonFlags(a);
    }

    void ccf()
    {
        flags.cf = flags.cf ? 0 : cbit;
        flags.hf = flags.cf;
        flags.nf = 0;
    }

    void scf()
    {
        flags.cf = cbit;
        flags.nf = 0;
        flags.hf = 0;
    }

    void daa()
    {
        u8 correction = 0;
        flags.cf = 0;

        if (flags.hf or (!flags.nf and (a & 0xF) > 9)) {
            correction |= 0x6;
        }

        if (flags.cf or (!flags.nf and (a & 0xF0) > 0x99)) {
            correction |= 0x60;
            flags.cf = cbit;
        }

        a += flags.nf ? -correction : correction;
        flags.pf = parity(a) ? pbit : 0;
        flags.sf = a & sbit;
        flags.zf = a == 0 ? zbit : 0;
        flags.yf = a & ybit;
        flags.xf = a & xbit;
    }

    // Rotate and Shift
    void rlca()
    {
        flags.cf = a & 0x80U ? cbit : 0;
        a = (a << 1U) | flags.cf;
        shiftFlags2(a);
    }

    void rla()
    {
        const u8 tmp {flags.cf};
        flags.cf = a & 0x80U ? cbit : 0;
        a = (a << 1U) | tmp;
        shiftFlags2(a);
    }

    void rrca()
    {
        flags.cf = a & 0x01U ? cbit : 0;
        a = (a >> 1U) | (flags.cf << 7U);
        shiftFlags2(a);
    }

    void rra()
    {
        const u8 tmp {flags.cf};
        flags.cf = a & 0x01U ? cbit : 0;
        a = (a >> 1U) | (tmp << 7U);
        shiftFlags2(a);
    }

    template<AddressMode mode1, AddressMode mode2>
    void rlc()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80 ? cbit : 0;
        val = (val << 1U) | flags.cf;
        shiftFlags1(val);
        setOperand<mode2>(val);
    }

    template<AddressMode mode1, AddressMode mode2>
    void rl()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        const u8 tmp {flags.cf};
        flags.cf = val & 0x80;
        val = (val << 1U) | tmp;
        shiftFlags1(val);
        setOperand<mode2>(val);
    }

    template<AddressMode mode1, AddressMode mode2>
    void rrc()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80 ? cbit : 0;
        val = (val >> 1U) | (flags.cf << 7U);
        shiftFlags1(val);
        setOperand<mode2>(val);
    }

    template<AddressMode mode1, AddressMode mode2>
    void rr()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        const u8 tmp {flags.cf};
        flags.cf = val & 0x80;
        val = (val >> 1U) | (tmp << 7U);
        shiftFlags1(val);
        setOperand<mode2>(val);
    }

    template<AddressMode mode1, AddressMode mode2>
    void sla()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80 ? cbit : 0;
        val <<= 1U;
        shiftFlags1(val);
        setOperand<mode2>(val);
    }

    template<AddressMode mode1, AddressMode mode2>
    void sll()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80;
        val = val << 1U | 1U;
        shiftFlags1(val);
        setOperand<mode2>(val);
    }

    template<AddressMode mode1, AddressMode mode2>
    void sra()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80 ? cbit : 0;
        const u8 tmp {static_cast<u8>(val & 0x80U)};
        val >>= 1U | tmp;
        shiftFlags1(val);
        setOperand<mode2>(val);
    }

    template<AddressMode mode1, AddressMode mode2>
    void srl()
    {
        u8 val {static_cast<u8>(getOperand<mode1>())};
        flags.cf = val & 0x80;
        val = val >> 1U;
        shiftFlags1(val);
        setOperand<mode2>(val);
    }

    void rld()
    {
        const u8 byte {read8(pair(regs.h, regs.l))};
        write(pair(regs.h, regs.l), static_cast<u8>(byte << 4U | a & 0xF));
        a &= ~0xF | byte & 0xF;
        shiftFlags1(a);
    }

    void rrd()
    {
        const u8 byte {read8(pair(regs.h, regs.l))};
        write(pair(regs.h, regs.l), static_cast<u8>(byte & ~0xF0 | a << 4U));
        a &= ~0xF | byte & 0xF;
        shiftFlags1(a);
    }

    // Bit Manipulation
    template<u8 mask, AddressMode mode>
    void bit()
    {
        const u8 operand {static_cast<u8>(getOperand<mode>())};
        flags.zf = operand & mask ? zbit : 0;
        flags.hf = hbit;
        flags.nf = 0;
        flags.yf = operand & ybit;
        flags.xf = operand & xbit;
    }

    template<u8 mask, AddressMode mode>
    void res() { setOperand<mode>(getOperand<mode>() & mask); }

    template<u8 mask, AddressMode mode>
    void set() { setOperand<mode>(getOperand<mode>() | mask); }

    // Jump, Call, and Return
    template<Condition cond>
    void jr()
    {
        if (getCondition<cond>()) {
            const s8 val {static_cast<s8>(read8(pc))};
            pc += val;
        }
    }

    template<Condition cond, AddressMode mode>
    void jp() { if (getCondition<cond>()) pc = getOperand<mode>(); }

    template<Condition cond, AddressMode mode>
    void call()
    {
        if (getCondition<cond>()) {
            const u16 operand {getOperand<mode>()};
            pushpc();
            pc = operand;
        }
    }

    void djnz()
    {
        if (--regs.b != 0 ) {
            const s8 val {static_cast<s8>(read8(pc))};
            pc += val;
        }
    }

    template<Condition cond>
    void ret() { if (getCondition<cond>()) pc = top(); }

    void retn() { pc = top(); iff1 = iff2; }

    template<u8 p>
    void rst()
    {
        pushpc();
        pc = p;
    }

    // Input/Output
    void in() { a = input(fetch8()); }

    template<AddressMode mode>
    void in()
    {
        const u8 c {input(regs.c)};
        if (mode != AddressMode::Flag)
            setOperand<mode>(c);
        shiftFlags1(c);
    }

    void ini()
    {
        write(pair(regs.h, regs.l), input(regs.c));
        blockIOFlags(1);
    }

    void inir()
    {
        ini();
        if (flags.zf)
            pc -= 2;
    }

    void ind()
    {
        write(pair(regs.h, regs.l), input(regs.c));
        blockIOFlags(-1);
    }

    void indr()
    {
        ind();
        if (flags.zf)
            pc -= 2;
    }

    void out() { output(fetch8(), a); }

    template<AddressMode mode>
    void out() { output(regs.c, getOperand<mode>()); }

    void outi()
    {
        output(regs.c, read8(pair(regs.h, regs.l)));
        blockIOFlags(1);
    }

    void otir()
    {
        outi();
        if (flags.zf)
            pc -= 2;
    }

    void outd()
    {
        output(regs.c, read8(pair(regs.h, regs.l)));
        blockIOFlags(-1);
    }

    void otdr()
    {
        outd();
        if (flags.zf)
            pc -= 2;
    }

    // CPU Control Group
    void nop() { }
    void halt() { } // TODO: implement this
    void di() { iff1 = false; }
    void ei() { iff1 = true; }
    template<u8 n>
    void im() { imode = n; }

    // Prefixes
    void preMisc()
    {
        const u8 opcode {fetch8()};
        (this->*miscInstruction[opcode])();
        requested -= miscCycles[opcode];
    }

    void preBit()
    {
        const u8 opcode {fetch8()};
        (this->*bitInstruction[opcode])();
        requested -= bitCycles[opcode];
    }

    template<AddressMode mode>
    void preIndex()
    {
        const u8 opcode {fetch8()};
        (this->*indexInstruction<mode>[opcode])();
        requested -= indexCycles[opcode];
    }

    template<AddressMode mode>
    void preIndexBit()
    {
        const u8 opcode {fetch8()};
        (this->*indexBitInstruction<mode>[opcode])();
        requested -= indexBitCycles[opcode];
    }

    // Main Instructions
    static constexpr std::array<void (z80::*)(), 256> instruction {
        &z80::nop, // $00: nop
        &z80::ld<AddressMode::RegisterBC, AddressMode::ImmediateEx>, // $01: ld bc, nn
        &z80::ld<AddressMode::RegisterIndirectBC, AddressMode::Accumulator>, // $02: ld (bc), a
        &z80::inc16<AddressMode::RegisterBC>, // $03: inc bc
        &z80::inc8<AddressMode::RegisterB>, // $04: inc b
        &z80::dec8<AddressMode::RegisterB>, // $05: dec b
        &z80::ld<AddressMode::RegisterB, AddressMode::Immediate>, // $06: ld b, n
        &z80::rlca, // $07: rlca,
        &z80::exaf, // $08: ex af, af'
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
        &z80::ld<AddressMode::Extended, AddressMode::RegisterHL>, // $22: ld (nn), hl
        &z80::inc16<AddressMode::RegisterHL>, // $23: inc hl
        &z80::inc8<AddressMode::RegisterH>, // $24: inc h
        &z80::dec8<AddressMode::RegisterH>, // $25: dec h
        &z80::ld<AddressMode::RegisterH, AddressMode::Immediate>, // $26: ld h, n
        &z80::daa, // $27: daa
        &z80::jr<Condition::Z>, // $28: jr z, d
        &z80::add16<AddressMode::RegisterHL, AddressMode::RegisterHL>, // $29: add hl, hl
        &z80::ld<AddressMode::RegisterHL, AddressMode::Extended>, // $2A: ld hl, (nn)
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
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterB>, // 40: ld b, b
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterC>, // 41: ld b, c
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterD>, // 42: ld b, d
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterE>, // 43: ld b, e
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterH>, // 44: ld b, h
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterL>, // 45: ld b, l
        &z80::ld<AddressMode::RegisterB, AddressMode::RegisterIndirectHL>, // 46: ld b, (hl)
        &z80::ld<AddressMode::RegisterB, AddressMode::Accumulator>, // 47: ld c, a
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterB>, // 48: ld c, b
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterC>, // 49: ld c, c
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterD>, // 4A: ld c, d
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterE>, // 4B: ld c, e
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterH>, // 4C: ld c, h
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterL>, // 4D: ld c, l
        &z80::ld<AddressMode::RegisterC, AddressMode::RegisterIndirectHL>, // 4E: ld c, (hl)
        &z80::ld<AddressMode::RegisterC, AddressMode::Accumulator>, // 4F: ld c, a
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterB>, // 50: ld d, b
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterC>, // 51: ld d, c
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterD>, // 52: ld d, d
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterE>, // 53: ld d, e
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterH>, // 54: ld d, h
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterL>, // 55: ld d, l
        &z80::ld<AddressMode::RegisterD, AddressMode::RegisterIndirectHL>, // 56: ld d, (hl)
        &z80::ld<AddressMode::RegisterD, AddressMode::Accumulator>, // 57: ld d, a
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterB>, // 58: ld e, b
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterC>, // 59: ld e, c
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterD>, // 5A: ld e, d
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterE>, // 5B: ld e, e
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterH>, // 5C: ld e, h
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterL>, // 5D: ld e, l
        &z80::ld<AddressMode::RegisterE, AddressMode::RegisterIndirectHL>, // 5E: ld e, (hl)
        &z80::ld<AddressMode::RegisterE, AddressMode::Accumulator>, // 5F: ld e, a
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterB>, // 60: ld h, b
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterC>, // 61: ld h, c
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterD>, // 62: ld h, d
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterE>, // 63: ld h, e
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterH>, // 64: ld h, h
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterL>, // 65: ld h, l
        &z80::ld<AddressMode::RegisterH, AddressMode::RegisterIndirectHL>, // 66: ld h, (hl)
        &z80::ld<AddressMode::RegisterH, AddressMode::Accumulator>, // 67: ld h, a
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterB>, // 68: ld l, b
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterC>, // 69: ld l, c
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterD>, // 6A: ld l, d
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterE>, // 6B: ld l, e
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterH>, // 6C: ld l, h
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterL>, // 6D: ld l, l
        &z80::ld<AddressMode::RegisterL, AddressMode::RegisterIndirectHL>, // 6E: ld l, (hl)
        &z80::ld<AddressMode::RegisterL, AddressMode::Accumulator>, // 6F: ld l, a
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterB>, // $70: ld (hl), b
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterC>, // $71: ld (hl), c
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterD>, // $72: ld (hl), d
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterE>, // $73: ld (hl), e
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterH>, // $74: ld (hl), h
        &z80::ld<AddressMode::RegisterIndirectHL, AddressMode::RegisterL>, // $75: ld (hl), l
        &z80::halt, // $76: ld (hl), b
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
        &z80::sbc8<AddressMode::Accumulator>, // $AF: sbc a, a
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
        &z80::out<AddressMode::Accumulator>, // $D3: out (n), a
        &z80::call<Condition::NC, AddressMode::ImmediateEx>, // $D4: call nc, nn
        &z80::push<AddressMode::RegisterDE>, // $D5: push de
        &z80::sub<AddressMode::Immediate>, // $D6: sub n
        &z80::rst<0x10U>, // $D7: rst 16
        &z80::ret<Condition::C>, // $D8: ret c
        &z80::exx, // $D9: exx
        &z80::jp<Condition::C, AddressMode::ImmediateEx>, // $DA: jp c, nn
        &z80::in, // $DB: int a, (n)
        &z80::call<Condition::C, AddressMode::ImmediateEx>, // $DC: call c, nn
        &z80::preIndex<AddressMode::RegisterIX>, // $DD: IX
        &z80::sbc8<AddressMode::Immediate>, // $DE: sbc a, n
        &z80::rst<0x18U>, // $DF: rst 24
        &z80::ret<Condition::PO>, // $E0: ret po
        &z80::pop<AddressMode::RegisterHL>, // $E1: pop hl
        &z80::jp<Condition::PO, AddressMode::ImmediateEx>, // $E2: jp po, nn
        &z80::exsp<AddressMode::RegisterHL>, // $E3: ex (sp), hl
        &z80::call<Condition::PO, AddressMode::ImmediateEx>, // $E4: call po, nn
        &z80::push<AddressMode::RegisterHL>, // $E5: push hl
        &z80::add8<AddressMode::Immediate>, // $E6: add n
        &z80::rst<0x20U>, // $E7: rst 32
        &z80::ret<Condition::PE>, // $E8: ret pe
        &z80::jp<Condition::Null, AddressMode::RegisterIndirectHL>, // $E9: jp (hl)
        &z80::jp<Condition::PE, AddressMode::ImmediateEx>, // $EA: jp pe, nn
        &z80::exde, // $EB: ex de, hl
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
        &z80::preIndex<AddressMode::RegisterIY>, // $FD: IY
        &z80::cp<AddressMode::Immediate>, // $FE: cp n
        &z80::rst<0x38U>,// $FF: rst 56
    };

    // Misc. Instructions ($ED)
    static constexpr std::array<void (z80::*)(), 256> miscInstruction {
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::in<AddressMode::RegisterB>, // $40: in b, (c)
        &z80::out<AddressMode::RegisterB>, // $41: out (c), b
        &z80::sbc16<AddressMode::RegisterHL, AddressMode::RegisterBC>, // $42: sbc, hl, bc
        &z80::ld<AddressMode::ImmediateEx, AddressMode::RegisterBC>, // $43: ld (nn), bc
        &z80::neg, // $44: neg
        &z80::retn, // $45: retn
        &z80::im<0U>, // $46: im 0
        &z80::ld<AddressMode::Interrupt, AddressMode::Accumulator>, // $47: ld i, a
        &z80::in<AddressMode::RegisterC>, // $48: in c, (c)
        &z80::out<AddressMode::RegisterC>, // $49: out (c), c
        &z80::adc16<AddressMode::RegisterHL, AddressMode::RegisterBC>, // $4A: adc hl, bc
        &z80::ld<AddressMode::RegisterBC, AddressMode::Extended>, // $4B: ld bc, (nn),
        &z80::nop,
        &z80::ret<Condition::Null>, // $4C: reti
        &z80::nop,
        &z80::ld<AddressMode::Refresh, AddressMode::Accumulator>, // $4F: ld r, a
        &z80::in<AddressMode::RegisterD>, // $50: in d, (c)
        &z80::out<AddressMode::RegisterD>, // $51: out (c), d
        &z80::sbc16<AddressMode::RegisterHL, AddressMode::RegisterDE>, // $52: sbc hl, de
        &z80::ld<AddressMode::ImmediateEx, AddressMode::RegisterDE>, // $53: ld (nn), de
        &z80::nop, &z80::nop,
        &z80::im<1U>, // $56: im 1
        &z80::ld<AddressMode::Accumulator, AddressMode::Interrupt>, // $57: ld a, i
        &z80::in<AddressMode::RegisterE>, // $58: in e, (c)
        &z80::out<AddressMode::RegisterE>, // $59: out (c), e
        &z80::adc16<AddressMode::RegisterHL, AddressMode::RegisterDE>, // $5A: adc hl, de
        &z80::ld<AddressMode::RegisterDE, AddressMode::Extended>, // $5B: ld de, (nn)
        &z80::nop, &z80::nop,
        &z80::im<2U>, // // $4E: im 2
        &z80::ld<AddressMode::Accumulator, AddressMode::Refresh>, // $4F: ld a, r
        &z80::in<AddressMode::RegisterH>, // $60: in h, (c)
        &z80::out<AddressMode::RegisterH>, // $61: out (c), h
        &z80::sbc16<AddressMode::RegisterHL, AddressMode::RegisterHL>, // $62: sbc hl, hl
        &z80::ld<AddressMode::ImmediateEx, AddressMode::RegisterHL>, // $63: ld (nn), hl
        &z80::nop, &z80::nop, &z80::nop,
        &z80::rrd, // $67: rrd
        &z80::in<AddressMode::RegisterL>, // $68: in l, (c)
        &z80::out<AddressMode::RegisterL>, // $69: out (c), l
        &z80::adc16<AddressMode::RegisterHL, AddressMode::RegisterHL>, // $6A: adc hl, hl
        &z80::ld<AddressMode::RegisterHL, AddressMode::Extended>, // $6B: ld hl, (nn)
        &z80::nop, &z80::nop, &z80::nop,
        &z80::rld, // $6F: rld
        &z80::in<AddressMode::Flag>, // $70: in h, (c)
        &z80::out<AddressMode::Null>, // $71: out (c), h
        &z80::sbc16<AddressMode::RegisterHL, AddressMode::RegisterSP>, // $72: sbc hl, sp
        &z80::ld<AddressMode::ImmediateEx, AddressMode::RegisterSP>, // $73: ld (nn), sp
        &z80::nop, &z80::nop, &z80::nop, &z80::nop,
        &z80::in<AddressMode::Accumulator>, // $78: in a, (c)
        &z80::out<AddressMode::Accumulator>, // $79: out (c), a
        &z80::adc16<AddressMode::RegisterHL, AddressMode::RegisterSP>, // $7A: adc hl, sp
        &z80::ld<AddressMode::RegisterSP, AddressMode::Extended>, // $7B: ld sp, (nn)
        &z80::nop, &z80::nop, &z80::nop, &z80::nop,
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

    // Bit Instructions ($CB)
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
        &z80::bit<0x01U, AddressMode::RegisterIndirectHL>, // $46: bit 0, (hl)
        &z80::bit<0x01U, AddressMode::Accumulator>, // $47: bit 0, a
        &z80::bit<0x02U, AddressMode::RegisterB>, // $48: bit 1, b
        &z80::bit<0x02U, AddressMode::RegisterC>, // $49: bit 1, c
        &z80::bit<0x02U, AddressMode::RegisterD>, // $4A: bit 1, d
        &z80::bit<0x02U, AddressMode::RegisterE>, // $4B: bit 1, e
        &z80::bit<0x02U, AddressMode::RegisterH>, // $4C: bit 1, h
        &z80::bit<0x02U, AddressMode::RegisterL>, // $4D: bit 1, l
        &z80::bit<0x02U, AddressMode::RegisterIndirectHL>, // $4E: bit 1, (hl)
        &z80::bit<0x02U, AddressMode::Accumulator>, // $4F: bit 1, a
        &z80::bit<0x04U, AddressMode::RegisterB>, // $50: bit 2, b
        &z80::bit<0x04U, AddressMode::RegisterC>, // $51: bit 2, c
        &z80::bit<0x04U, AddressMode::RegisterD>, // $52: bit 2, d
        &z80::bit<0x04U, AddressMode::RegisterE>, // $53: bit 2, e
        &z80::bit<0x04U, AddressMode::RegisterH>, // $54: bit 2, h
        &z80::bit<0x04U, AddressMode::RegisterL>, // $55: bit 2, l
        &z80::bit<0x04U, AddressMode::RegisterIndirectHL>, // $56: bit 2, (hl)
        &z80::bit<0x04U, AddressMode::Accumulator>, // $57: bit 2, a
        &z80::bit<0x08U, AddressMode::RegisterB>, // $58: bit 3, b
        &z80::bit<0x08U, AddressMode::RegisterC>, // $59: bit 3, c
        &z80::bit<0x08U, AddressMode::RegisterD>, // $5A: bit 3, d
        &z80::bit<0x08U, AddressMode::RegisterE>, // $5B: bit 3, e
        &z80::bit<0x08U, AddressMode::RegisterH>, // $5C: bit 3, h
        &z80::bit<0x08U, AddressMode::RegisterL>, // $5D: bit 3, l
        &z80::bit<0x08U, AddressMode::RegisterIndirectHL>, // $5E: bit 3, (hl)
        &z80::bit<0x08U, AddressMode::Accumulator>, // $5F: bit 3, a
        &z80::bit<0x10U, AddressMode::RegisterB>, // $60: bit 4, b
        &z80::bit<0x10U, AddressMode::RegisterC>, // $61: bit 4, c
        &z80::bit<0x10U, AddressMode::RegisterD>, // $62: bit 4, d
        &z80::bit<0x10U, AddressMode::RegisterE>, // $63: bit 4, e
        &z80::bit<0x10U, AddressMode::RegisterH>, // $64: bit 4, h
        &z80::bit<0x10U, AddressMode::RegisterL>, // $65: bit 4, l
        &z80::bit<0x10U, AddressMode::RegisterIndirectHL>, // $66: bit 4, (hl)
        &z80::bit<0x10U, AddressMode::Accumulator>, // $67: bit 4, a
        &z80::bit<0x20U, AddressMode::RegisterB>, // $68: bit 5, b
        &z80::bit<0x20U, AddressMode::RegisterC>, // $69: bit 5, c
        &z80::bit<0x20U, AddressMode::RegisterD>, // $6A: bit 5, d
        &z80::bit<0x20U, AddressMode::RegisterE>, // $6B: bit 5, e
        &z80::bit<0x20U, AddressMode::RegisterH>, // $6C: bit 5, h
        &z80::bit<0x20U, AddressMode::RegisterL>, // $6D: bit 5, l
        &z80::bit<0x20U, AddressMode::RegisterIndirectHL>, // $6E: bit 5, (hl)
        &z80::bit<0x20U, AddressMode::Accumulator>, // $6F: bit 5, a
        &z80::bit<0x40U, AddressMode::RegisterB>, // $70: bit 6, b
        &z80::bit<0x40U, AddressMode::RegisterC>, // $71: bit 6, c
        &z80::bit<0x40U, AddressMode::RegisterD>, // $72: bit 6, d
        &z80::bit<0x40U, AddressMode::RegisterE>, // $73: bit 6, e
        &z80::bit<0x40U, AddressMode::RegisterH>, // $74: bit 6, h
        &z80::bit<0x40U, AddressMode::RegisterL>, // $75: bit 6, l
        &z80::bit<0x40U, AddressMode::RegisterIndirectHL>, // $76: bit 6, (hl)
        &z80::bit<0x40U, AddressMode::Accumulator>, // $77: bit 6, a
        &z80::bit<0x80U, AddressMode::RegisterB>, // $78: bit 7, b
        &z80::bit<0x80U, AddressMode::RegisterC>, // $79: bit 7, c
        &z80::bit<0x80U, AddressMode::RegisterD>, // $7A: bit 7, d
        &z80::bit<0x80U, AddressMode::RegisterE>, // $7B: bit 7, e
        &z80::bit<0x80U, AddressMode::RegisterH>, // $7C: bit 7, h
        &z80::bit<0x80U, AddressMode::RegisterL>, // $7D: bit 7, l
        &z80::bit<0x80U, AddressMode::RegisterIndirectHL>, // $7E: bit 7, (hl)
        &z80::bit<0x80U, AddressMode::Accumulator>, // $7F: bit 7, a
        &z80::res<0xFEU, AddressMode::RegisterB>, // $80: res 0, b
        &z80::res<0xFEU, AddressMode::RegisterC>, // $81: res 0, c
        &z80::res<0xFEU, AddressMode::RegisterD>, // $82: res 0, d
        &z80::res<0xFEU, AddressMode::RegisterE>, // $83: res 0, e
        &z80::res<0xFEU, AddressMode::RegisterH>, // $84: res 0, h
        &z80::res<0xFEU, AddressMode::RegisterL>, // $85: res 0, l
        &z80::res<0xFEU, AddressMode::RegisterIndirectHL>, // $86: res 0, (hl)
        &z80::res<0xFEU, AddressMode::Accumulator>, // $87: res 0, a
        &z80::res<0xFDU, AddressMode::RegisterB>, // $88: res 1, b
        &z80::res<0xFDU, AddressMode::RegisterC>, // $89: res 1, c
        &z80::res<0xFDU, AddressMode::RegisterD>, // $8A: res 1, d
        &z80::res<0xFDU, AddressMode::RegisterE>, // $8B: res 1, e
        &z80::res<0xFDU, AddressMode::RegisterH>, // $8C: res 1, h
        &z80::res<0xFDU, AddressMode::RegisterL>, // $8D: res 1, l
        &z80::res<0xFDU, AddressMode::RegisterIndirectHL>, // $8E: res 1, (hl)
        &z80::res<0xFDU, AddressMode::Accumulator>, // $8F: res 1, a
        &z80::res<0xFBU, AddressMode::RegisterB>, // $90: res 2, b
        &z80::res<0xFBU, AddressMode::RegisterC>, // $91: res 2, c
        &z80::res<0xFBU, AddressMode::RegisterD>, // $92: res 2, d
        &z80::res<0xFBU, AddressMode::RegisterE>, // $93: res 2, e
        &z80::res<0xFBU, AddressMode::RegisterH>, // $94: res 2, h
        &z80::res<0xFBU, AddressMode::RegisterL>, // $95: res 2, l
        &z80::res<0xFBU, AddressMode::RegisterIndirectHL>, // $96: res 2, (hl)
        &z80::res<0xFBU, AddressMode::Accumulator>, // $97: res 2, a
        &z80::res<0xF7U, AddressMode::RegisterB>, // $98: res 3, b
        &z80::res<0xF7U, AddressMode::RegisterC>, // $99: res 3, c
        &z80::res<0xF7U, AddressMode::RegisterD>, // $9A: res 3, d
        &z80::res<0xF7U, AddressMode::RegisterE>, // $9B: res 3, e
        &z80::res<0xF7U, AddressMode::RegisterH>, // $9C: res 3, h
        &z80::res<0xF7U, AddressMode::RegisterL>, // $9D: res 3, l
        &z80::res<0xF7U, AddressMode::RegisterIndirectHL>, // $9E: res 3, (hl)
        &z80::res<0xF7U, AddressMode::Accumulator>, // $9F: res 3, a
        &z80::res<0xEFU, AddressMode::RegisterB>, // $A0: res 4, b
        &z80::res<0xEFU, AddressMode::RegisterC>, // $A1: res 4, c
        &z80::res<0xEFU, AddressMode::RegisterD>, // $A2: res 4, d
        &z80::res<0xEFU, AddressMode::RegisterE>, // $A3: res 4, e
        &z80::res<0xEFU, AddressMode::RegisterH>, // $A4: res 4, h
        &z80::res<0xEFU, AddressMode::RegisterL>, // $A5: res 4, l
        &z80::res<0xEFU, AddressMode::RegisterIndirectHL>, // $A6: res 4, (hl)
        &z80::res<0xEFU, AddressMode::Accumulator>, // $A7: res 4, a
        &z80::res<0xDFU, AddressMode::RegisterB>, // $A8: res 5, b
        &z80::res<0xDFU, AddressMode::RegisterC>, // $A9: res 5, c
        &z80::res<0xDFU, AddressMode::RegisterD>, // $AA: res 5, d
        &z80::res<0xDFU, AddressMode::RegisterE>, // $AB: res 5, e
        &z80::res<0xDFU, AddressMode::RegisterH>, // $AC: res 5, h
        &z80::res<0xDFU, AddressMode::RegisterL>, // $AD: res 5, l
        &z80::res<0xDFU, AddressMode::RegisterIndirectHL>, // $AE: res 5, (hl)
        &z80::res<0xDFU, AddressMode::Accumulator>, // $AF: res 5, a
        &z80::res<0xBFU, AddressMode::RegisterB>, // $B0: res 6, b
        &z80::res<0xBFU, AddressMode::RegisterC>, // $B1: res 6, c
        &z80::res<0xBFU, AddressMode::RegisterD>, // $B2: res 6, d
        &z80::res<0xBFU, AddressMode::RegisterE>, // $B3: res 6, e
        &z80::res<0xBFU, AddressMode::RegisterH>, // $B4: res 6, h
        &z80::res<0xBFU, AddressMode::RegisterL>, // $B5: res 6, l
        &z80::res<0xBFU, AddressMode::RegisterIndirectHL>, // $B6: res 6, (hl)
        &z80::res<0xBFU, AddressMode::Accumulator>, // $B7: res 6, a
        &z80::res<0x7FU, AddressMode::RegisterB>, // $B8: res 7, b
        &z80::res<0x7FU, AddressMode::RegisterC>, // $B9: res 7, c
        &z80::res<0x7FU, AddressMode::RegisterD>, // $BA: res 7, d
        &z80::res<0x7FU, AddressMode::RegisterE>, // $BB: res 7, e
        &z80::res<0x7FU, AddressMode::RegisterH>, // $BC: res 7, h
        &z80::res<0x7FU, AddressMode::RegisterL>, // $BD: res 7, l
        &z80::res<0x7FU, AddressMode::RegisterIndirectHL>, // $BE: res 7, (hl)
        &z80::res<0x7FU, AddressMode::Accumulator>, // $BF: res 7, a
        &z80::set<0x01U, AddressMode::RegisterB>, // $C0: set 0, b
        &z80::set<0x01U, AddressMode::RegisterC>, // $C1: set 0, c
        &z80::set<0x01U, AddressMode::RegisterD>, // $C2: set 0, d
        &z80::set<0x01U, AddressMode::RegisterE>, // $C3: set 0, e
        &z80::set<0x01U, AddressMode::RegisterH>, // $C4: set 0, h
        &z80::set<0x01U, AddressMode::RegisterL>, // $C5: set 0, l
        &z80::set<0x01U, AddressMode::RegisterIndirectHL>, // $C6: set 0, (hl)
        &z80::set<0x01U, AddressMode::Accumulator>, // $C7: set 0, a
        &z80::set<0x02U, AddressMode::RegisterB>, // $C8: set 1, b
        &z80::set<0x02U, AddressMode::RegisterC>, // $C9: set 1, c
        &z80::set<0x02U, AddressMode::RegisterD>, // $CA: set 1, d
        &z80::set<0x02U, AddressMode::RegisterE>, // $CB: set 1, e
        &z80::set<0x02U, AddressMode::RegisterH>, // $CC: set 1, h
        &z80::set<0x02U, AddressMode::RegisterL>, // $CD: set 1, l
        &z80::set<0x02U, AddressMode::RegisterIndirectHL>, // $CE: set 1, (hl)
        &z80::set<0x02U, AddressMode::Accumulator>, // $CF: set 1, a
        &z80::set<0x04U, AddressMode::RegisterB>, // $D0: set 2, b
        &z80::set<0x04U, AddressMode::RegisterC>, // $D1: set 2, c
        &z80::set<0x04U, AddressMode::RegisterD>, // $D2: set 2, d
        &z80::set<0x04U, AddressMode::RegisterE>, // $D3: set 2, e
        &z80::set<0x04U, AddressMode::RegisterH>, // $D4: set 2, h
        &z80::set<0x04U, AddressMode::RegisterL>, // $D5: set 2, l
        &z80::set<0x04U, AddressMode::RegisterIndirectHL>, // $D6: set 2, (hl)
        &z80::set<0x04U, AddressMode::Accumulator>, // $D7: set 2, a
        &z80::set<0x08U, AddressMode::RegisterB>, // $D8: set 3, b
        &z80::set<0x08U, AddressMode::RegisterC>, // $D9: set 3, c
        &z80::set<0x08U, AddressMode::RegisterD>, // $DA: set 3, d
        &z80::set<0x08U, AddressMode::RegisterE>, // $DB: set 3, e
        &z80::set<0x08U, AddressMode::RegisterH>, // $DC: set 3, h
        &z80::set<0x08U, AddressMode::RegisterL>, // $DD: set 3, l
        &z80::set<0x08U, AddressMode::RegisterIndirectHL>, // $DE: set 3, (hl)
        &z80::set<0x08U, AddressMode::Accumulator>, // $DF: set 3, a
        &z80::set<0x10U, AddressMode::RegisterB>, // $E0: set 4, b
        &z80::set<0x10U, AddressMode::RegisterC>, // $E1: set 4, c
        &z80::set<0x10U, AddressMode::RegisterD>, // $E2: set 4, d
        &z80::set<0x10U, AddressMode::RegisterE>, // $E3: set 4, e
        &z80::set<0x10U, AddressMode::RegisterH>, // $E4: set 4, h
        &z80::set<0x10U, AddressMode::RegisterL>, // $E5: set 4, l
        &z80::set<0x10U, AddressMode::RegisterIndirectHL>, // $E6: set 4, (hl)
        &z80::set<0x10U, AddressMode::Accumulator>, // $E7: set 4, a
        &z80::set<0x20U, AddressMode::RegisterB>, // $E8: set 5, b
        &z80::set<0x20U, AddressMode::RegisterC>, // $E9: set 5, c
        &z80::set<0x20U, AddressMode::RegisterD>, // $EA: set 5, d
        &z80::set<0x20U, AddressMode::RegisterE>, // $EB: set 5, e
        &z80::set<0x20U, AddressMode::RegisterH>, // $EC: set 5, h
        &z80::set<0x20U, AddressMode::RegisterL>, // $ED: set 5, l
        &z80::set<0x20U, AddressMode::RegisterIndirectHL>, // $EE: set 5, (hl)
        &z80::set<0x20U, AddressMode::Accumulator>, // $EF: set 5, a
        &z80::set<0x40U, AddressMode::RegisterB>, // $F0: set 6, b
        &z80::set<0x40U, AddressMode::RegisterC>, // $F1: set 6, c
        &z80::set<0x40U, AddressMode::RegisterD>, // $F2: set 6, d
        &z80::set<0x40U, AddressMode::RegisterE>, // $F3: set 6, e
        &z80::set<0x40U, AddressMode::RegisterH>, // $F4: set 6, h
        &z80::set<0x40U, AddressMode::RegisterL>, // $F5: set 6, l
        &z80::set<0x40U, AddressMode::RegisterIndirectHL>, // $F6: set 6, (hl)
        &z80::set<0x40U, AddressMode::Accumulator>, // $F7: set 6, a
        &z80::set<0x80U, AddressMode::RegisterB>, // $F8: set 7, b
        &z80::set<0x80U, AddressMode::RegisterC>, // $F9: set 7, c
        &z80::set<0x80U, AddressMode::RegisterD>, // $FA: set 7, d
        &z80::set<0x80U, AddressMode::RegisterE>, // $FB: set 7, e
        &z80::set<0x80U, AddressMode::RegisterH>, // $FC: set 7, h
        &z80::set<0x80U, AddressMode::RegisterL>, // $FD: set 7, l
        &z80::set<0x80U, AddressMode::RegisterIndirectHL>, // $FE: set 7, (hl)
        &z80::set<0x80U, AddressMode::Accumulator>, // $FF: set 7, a
    };

    // Index Instructions ($DD/$FD)
    template<AddressMode mode>
    static constexpr std::array<void (z80::*)(), 256> indexInstruction {

    };

    // Index Bit instructions ($DDCB/$FDCB)
    template<AddressMode mode>
    static constexpr std::array<void (z80::*)(), 256> indexBitInstruction {

    };

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
        12, 12, 15, 20, 8, 14, 8, 9, 12, 12, 15, 20, 4, 14, 4, 9,
        12, 12, 15, 20, 4, 4, 8, 9, 12, 12, 15, 20, 4, 4, 8, 9,
        12, 12, 15, 20, 4, 4, 4, 18, 12, 12, 15, 20, 4, 4, 4, 18,
        12, 12, 15, 20, 4, 4, 4, 4, 12, 12, 15, 20, 4, 4, 4, 4,
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
        4, 4, 4, 4, 8, 8, 11, 4, 4, 15, 4, 4, 8, 8, 11, 4,
        4, 4, 4, 4, 8, 8, 11, 4, 4, 15, 4, 4, 8, 8, 11, 4,
        4, 14, 10, 20, 8, 8, 11, 4, 4, 15, 20, 10, 8, 8, 11, 4,
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

    bool iff1 {false}, iff2 {false}; // interrupt latch
    u8 imode {}; // interrupt mode
    bool halted {false};
    int requested {};
    u8 a2 {};
    Registers regs2 {};
    Flags flags2 {};
    Memory memory {};
    IO io {};
};

template<typename Memory, typename IO>
int z80<Memory, IO>::run(const int cycles)
{
    requested = cycles;
    while (requested > 0) { tick(); }
    return requested;
}

template<typename Memory, typename IO>
void z80<Memory, IO>::tick()
{
    u8 opcode {fetch8()};
    (this->*instruction[opcode])();
    requested -= cycles[opcode];
}

#endif //Z80_LIBRARY_H