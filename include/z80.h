#ifndef Z80_LIBRARY_H
#define Z80_LIBRARY_H

#include <cstdint>

class z80 {
public:
    int step();
    virtual void writeByte();
    virtual void writeWord();
    virtual std::uint8_t readByte();
    virtual std::uint16_t readWord();
private:

};

int z80::step()
{
    return 0;
}

#endif //Z80_LIBRARY_H
