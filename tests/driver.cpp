#include <fstream>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <format>
#include <string>
#include <array>
#include <chrono>
#include "z80.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

static constexpr std::uint16_t start {0x100};
static constexpr int clockSpeed {static_cast<int>(2.5e6)}; // 2.5 MHz
static constexpr int cycles {clockSpeed / 60};
static constexpr bool logging {false}; // log the cpu state every instruction **NOTE: will output dozens of gb of data!**
static constexpr bool verbose {false}; // output every read/write instruction, address, and value read/written

struct Memory {
    std::array<std::uint8_t, 0x10000> ram {};
    bool exit {false}, cpm {true};

    [[nodiscard]] std::uint8_t read8(const std::uint16_t addr) const
    {
        if constexpr (verbose) std::cout << std::format("\tREAD: address=${:0>4X}, val=${:0>2X}\n", addr, ram[addr]);
        return ram[addr];
    }
    [[nodiscard]] std::uint16_t read16(const std::uint16_t addr) const
    {
        if constexpr (verbose) std::cout << std::format("\tREAD: address=${:0>4X}, val=${:0>4X}\n", addr, ram[addr + 1] << 8 | ram[addr]);
        return ram[static_cast<std::uint16_t>(addr + 1)] << 8 | ram[addr];
    }
    void write8(const std::uint16_t addr, const std::uint8_t val)
    {
        if constexpr (verbose) std::cout << std::format("\tWRITE: address=${:0>4X}, val=${:0>2X}\n", addr, val);
        ram[addr] = val;
    }
    void write16(const std::uint16_t addr, const std::uint16_t val)
    {
        if constexpr (verbose) std::cout << std::format("\tWRITE: address=${:0>4X}, val=${:0>4X}\n", addr, val);
        ram[addr] = val & 0xFF; ram[static_cast<std::uint16_t>(addr + 1)] = val >> 8;
    }

    [[nodiscard]] std::uint8_t input(const z80<Memory>* cpu, const std::uint8_t port) const
    {
        if (cpm) {
            if (const std::uint8_t operation {cpu->regs.c}; operation == 2) {
                // print a character stored in E
                std::cout << static_cast<char>(cpu->regs.e);
            } else if (operation == 9) {
                // print from memory at (DE) until '$' char
                std::uint16_t addr {static_cast<std::uint16_t>(cpu->regs.d << 8U | cpu->regs.e)};
                do {
                    std::cout << static_cast<char>(ram[addr++]);
                } while (ram[addr] != '$');
            }
            return 0xFF;
        }

        return ram[port];
    }
    void output(const z80<Memory>* cpu, const std::uint8_t port, const std::uint8_t val) { exit = true; }
};

int load(z80<Memory>& z, std::string path)
{
    std::ifstream file {path, std::ios::binary};

    if (file.fail()) {
        std::cerr << std::format("error: failure when opening file '{:s}'.\n", path);
        return -1;
    }

    if (!file.is_open()) {
        std::cerr << std::format("error: unable to open file '{:s}'.\n", path);
        return -2;
    }
    file.read(reinterpret_cast<char*>(&z.memory.ram[start]), z.memory.ram.size());

    return 0;
}

void log(const z80<Memory>& z, const unsigned long long cycle)
{
    std::cout << z.toString(cycle) << '\n';

}

std::string formatTime(const std::chrono::steady_clock::time_point begin, const std::chrono::steady_clock::time_point end)
{
    if (const auto ms {std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)}; ms.count() >= 60 * 1e3L) {
        const std::string min {std::to_string(std::chrono::duration_cast<std::chrono::minutes>(ms).count())};
        std::string sec {std::to_string(std::chrono::duration_cast<std::chrono::seconds>(ms).count())};
        return  min + '.' + sec.erase(0, 1) + " min";
    } else {
        if (ms.count() >= 1e3L) {
            const std::string sec {std::to_string(std::chrono::duration_cast<std::chrono::seconds>(ms).count())};
            return  sec + '.' + std::to_string(ms.count()).erase(0, 1) + " sec";
        }
        return std::to_string(ms.count()) + " ms";
    }
}

void test(z80<Memory>& z, std::string rom, const unsigned long long expected)
{
    if (load(z, rom) < 0)
        return;

    std::cout << std::format("*** TEST: {:s}\n", rom);

    // inject output instruction at 0x0 to stop the test
    z.memory.ram[0x0] = 0xD3; // $D3 = out (n), a
    z.memory.ram[0x1] = 0x00; // (n) = 0

    // inject input instruction at 0x5 to output some characters
    z.memory.ram[0x5] = 0xDB; // $DB = in a, (n)
    z.memory.ram[0x6] = 0x00; // (n) = 0

    // inject ret instruction at 0x7 to return from syscall
    z.memory.ram[0x7] = 0xC9; // $C9 = ret

    // reset the cpu
    z.reset();
    z.pc = start; z.memory.exit = false;

    unsigned long long count {0}; // holds number of executed instructions
    unsigned long long executed {0}; // holds number of executed cycles
    const std::chrono::steady_clock::time_point begin {std::chrono::steady_clock::now()};

    // run the test
    if constexpr (logging) log(z, 0);
    while(!z.memory.exit) {
        z.requested = cycles - z.requested;

        while(z.requested > 0 and !z.memory.exit) {
            const int tmp {z.requested};
            z.step();
            ++count;
            executed += tmp - z.requested;
            if constexpr(logging) log(z, executed);
        }
    }

    // output results
    unsigned long long diff {expected > executed ? expected - executed : executed - expected};
    std::cout << std::format("\n*** {:d} instructions executed on {:d} cycles (expected={:d}, diff={:d}) in {:s}\n\n",
           count, executed, expected, diff, formatTime(begin, std::chrono::steady_clock::now()));
}

bool equalityBool(const bool expected, const bool actual, const std::string& name)
{
    if (expected != actual)
        std::cout << std::format("{:s}: expected {:s} got {:s}\n", name, expected ? "true" : "false", actual ? "true" : "false");
    return expected == actual;
}

bool equality8(const std::uint8_t expected, const std::uint8_t actual, const std::string& name)
{
    if (expected != actual)
        std::cout << std::format("{:s}: expected {:d} got {:d}\n", name, expected, actual);
    return expected == actual;
}

bool equality16(const std::uint16_t expected, const std::uint16_t actual, const std::string& name)
{
    if (expected != actual)
        std::cout << std::format("{:s}: expected {:d} got {:d}\n", name, expected, actual);
    return expected == actual;
}

bool unitTest(z80<Memory>& z, const std::filesystem::path& path)
{
    std::ifstream f {path};
    json j = json::parse(f);

    const std::chrono::steady_clock::time_point begin {std::chrono::steady_clock::now()};
    bool passed {true};
    auto& lastTest {j[0]};

    for (const auto& test : j) {
        if (!passed) break;

        // set initial processor state from test
        z.reset();
        z.pc = test["initial"]["pc"];
        z.wz = test["initial"]["wz"];
        z.sp = test["initial"]["sp"];
        z.a = test["initial"]["a"];
        z.setf(z.flags, test["initial"]["f"]);
        z.regs.b = test["initial"]["b"];
        z.regs.c = test["initial"]["c"];
        z.regs.d = test["initial"]["d"];
        z.regs.e = test["initial"]["e"];
        z.regs.h = test["initial"]["h"];
        z.regs.l = test["initial"]["l"];
        z.r = test["initial"]["r"];
        z.i = test["initial"]["i"];
        z.ix = test["initial"]["ix"];
        z.iy = test["initial"]["iy"];
        z.imode = test["initial"]["im"];
        z.iff1 = test["initial"]["iff1"] != 0;
        z.iff2 = test["initial"]["iff2"] != 0;
        z.flagsWereModified = test["initial"]["q"] != 0;

        // set initial RAM state from test
        for (const auto& data : test["initial"]["ram"])
            z.memory.ram[data[0]] = data[1];

        // set PORTs for input
        if (test.contains("ports")) {
            for (const auto& data : test["ports"]) {
                if (data[2] != "r") continue;
                std::uint8_t port {data[0]};
                z.memory.ram[port] = data[1];
            }
        }

        // step the processor
        z.step();

        // compare final RAM state to test and report any errors
        for (const auto& data : test["final"]["ram"])
            passed &= equality8(data[1], z.memory.ram[data[0]], "RAM");

        // compare final processor state to test and report any errors
        passed &= equality16(test["final"]["pc"], z.pc, "PC");
        passed &= equality16(test["final"]["wz"], z.wz, "WZ");
        passed &= equality16(test["final"]["sp"], z.sp, "SP");
        passed &= equality8(test["final"]["a"], z.a, "A");
        passed &= equality8(test["final"]["f"], z.getf(), "F");
        passed &= equality8(test["final"]["b"], z.regs.b, "B");
        passed &= equality8(test["final"]["c"], z.regs.c, "C");
        passed &= equality8(test["final"]["d"], z.regs.d, "D");
        passed &= equality8(test["final"]["e"], z.regs.e, "E");
        passed &= equality8(test["final"]["h"], z.regs.h, "H");
        passed &= equality8(test["final"]["l"], z.regs.l, "L");
        passed &= equality8(test["final"]["r"], z.r, "R");
        passed &= equality8(test["final"]["i"], z.i, "I");
        passed &= equality16(test["final"]["ix"], z.ix, "IX");
        passed &= equality16(test["final"]["iy"], z.iy, "IY");
        passed &= equality8(test["final"]["im"], z.imode, "IM");
        passed &= equalityBool(test["final"]["iff1"] != 0, z.iff1, "IFF1");
        passed &= equalityBool(test["final"]["iff2"] != 0, z.iff2, "IFF2");

        lastTest = test;
    }

    if (!passed)
        std::cout << "FAILED " << path.string() << ' ' << lastTest["name"] << "\nCPU STATE=" << z.toString() << '\n' << "TEST CASE=" << lastTest.dump() << "\n\n";

    return passed;
}

int main(int argc, char** argv)
{
    const std::string env {argv[0]};
    z80<Memory> z {};

    const std::chrono::steady_clock::time_point begin {std::chrono::steady_clock::now()};

    // run test roms (https://github.com/superzazu/z80/tree/master/roms)
    test(z, "tests/test-roms/prelim.com", 8721ULL);
    test(z, "tests/test-roms/zexdoc.cim", 46734978649ULL);
    test(z, "tests/test-roms/zexall.cim", 46734978649ULL);

    // run unit tests
    /*
     *   Get the *.json files from here (https://github.com/raddad772/jsmoo/tree/main/misc/tests/GeneratedTests/z80)
     *   Known to fail the following:
     *       - Unimplemented: ED 77, ED 76, ED 66, ED 4E, ED 6E, ED 7E, ED 7F
     *       - I/O Related: 08, D9, DD 08, DD D9, FD 08, FD 09,
     *       - LDI/LDIR/LDD/LDDR/CPI/CPIR/CPD/CPDR/INI/INIR/IND/INDR/OUTI/OTIR/OUTD/OTDR: Flags XF and YF
     */
    /*
    z.memory.cpm = false;
    std::cout << "*** Running unit tests\n";
    int passed {0}, failed {0};
    const std::chrono::steady_clock::time_point begin2 {std::chrono::steady_clock::now()};
    for (const auto & entry : std::filesystem::directory_iterator("tests/unit-tests"))
        if (entry.path().string().find(".json") != std::string::npos) unitTest(z, entry.path()) ? ++passed : ++failed;

    std::cout << std::format("*** Unit testing complete in {:s}. Passed {:d}, Failed {:d}\n\n", formatTime(begin2, std::chrono::steady_clock::now()), passed, failed);
    */

    std::cout << "Done. Total time elapsed " << formatTime(begin, std::chrono::steady_clock::now()) << std::endl;
}