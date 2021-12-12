#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;

class Crc {
  public:
    Crc() = default;
    ~Crc() = default;

    void init()      { mVal = 0xffff; }
    void add( u8 b ) { mVal = (mVal >> 8) ^ sTab[(mVal ^ b) & 0xff]; }
    u16  get() const { return ~mVal; }

  private:
    static u16 const sTab[0x100];
    u16 mVal;
};

