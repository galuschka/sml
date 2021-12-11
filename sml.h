
#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

enum class Byte {
    MsgEnd   = 0,
    Skip     = 1,  // optional
    Begin    = 1,
    End      = 0x1a,
    Escape   = 0x1b,
    SizeMask = 0x0f,
    TypeMask = 0x70,
    OtherBit = 0x80,
};
enum class Type {
    ByteStr  =    0,
    Invalid1 = 0x10,
    Invalid2 = 0x20,
    Invalid3 = 0x30,
    Boolean  = 0x40,
    Integer  = 0x50,
    Unsigned = 0x60,
    List     = 0x70,
};
/*
enum class Size {
    u8  = sizeof(u8),
    u16 = sizeof(u16),
    u32 = sizeof(u32),
    u64 = sizeof(u64),
};
*/

class SmlObj;

class Sml
{
  public:
    enum class Status {
        EscBegin,
        Begin,
        Parse,
        EscEnd,
        End
    };
    Sml();
    ~Sml();

    void parse( u8 byte );  // parse next input byte
    void start();           // start parsing objects (behind esc begin)
    void show();            // dump the struct

  private:
    Status   mStatus;
    u8       mEscCnt;  // number of escape chars read
    SmlObj * mRoot;    // root object: list of 3 or more objects
    SmlObj * mParsing; // pointer to currently parsed object
    u16      mCrcCalc; // calculated by input data
    u16      mCrcRead; // CRC from input data
};

class SmlObj
{
  public:
    SmlObj();
    SmlObj( u8 byte, SmlObj * parent );
    ~SmlObj();

    union Value {
        Value() { _u64 = 0LL; }
        u8     * byteStr;
        i8       _i8;
        i16      _i16;
        i32      _i32;
        i64      _i64;
        u8       _u8;
        u16      _u16;
        u32      _u32;
        u64      _u64;
        SmlObj * list;  // sub list
    }        mVal;
    Type     mType;     // Type::...
    u8       mSize;     // number of ... (or 0 when root list)
    u8       mCount;    // how many bytes / elements read so far
    SmlObj * mNext;     // next element in same list (nil when last)
    SmlObj * mParent;   // parent list (the list containing this)

    SmlObj * parse( u8 byte );  // next input byte -> true: complete
    void     showobj( u8 indent );
};
