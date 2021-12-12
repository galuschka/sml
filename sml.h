
#pragma once

#include <stdint.h>
#include "crc16.h"

typedef uint8_t  	u8;
typedef uint16_t 	u16;
typedef uint32_t	u32;
typedef uint64_t 	u64;

typedef int8_t  	i8;
typedef int16_t 	i16;
typedef int32_t		i32;
typedef int64_t		i64;

typedef u8			idx;

const idx cMaxNofObj  	= 150;  // <- adjust according your needs
const u8  cMaxListDepth  = 10;  // while parsing nested list, we have to save old element counter
const u8  cMinNofEscBegin = 3;  // ok, when Byte::Begin after 3 Byte::Escape
const u8  cNofEscBegin    = 4;  // usual number of Byte::Escape
const u8  cNofBegin       = 4;  // number of Byte::Begin
const u8  cNofEscEnd      = 4;  // number of Byte::Escape before Byte::End

namespace Byte {
enum Byte {
    MsgEnd   = 0,
    Skip     = 1,  // optional
    Begin    = 1,
    End      = 0x1a,
    Escape   = 0x1b,
    SizeMask = 0x0f,
    TypeMask = 0x70,
    OtherBit = 0x80,
    TypeShift = 4,  // type >> 4 is basic type value
};
}
namespace Type {
enum Type {
    ByteStr  =    0,
    Invalid1 = 0x10,
    Invalid2 = 0x20,
    Invalid3 = 0x30,
    Boolean  = 0x40,
    Integer  = 0x50,
    Unsigned = 0x60,
    List     = 0x70,
};
}

namespace Err {
enum Err {
    NoError = 0,
    OutOfMemory,	// cMaxNofObj too small
    InvalidType,
    CrcError,
};
}

class Sml;

class SmlObj
{
  public:
    idx      mParent;   // parent list (the list containing this)
    idx      mNext;     // next element in same list (nil when last)
    u8       mTypeSize; // type and number of ... (or 0 when root list)
    idx      mVal;      // byte value or index to list or index to value
};

class Sml
{
  public:
    Sml();
    virtual ~Sml() = 0;

    void parse( u8 byte );  // parse next input byte
    void start();           // start parsing objects (behind esc begin)
    void dump();            // dump the struct
    idx  abort( u8 err );   // set to abort parsing (on type error)

    virtual void onReady( u8 err, u8 byte ) = 0;  // method called on parsing complete/abort

    idx 	 newObj(  u8 byte, idx parent );
    idx 	 newData( u8 size );
    SmlObj * obj( idx i ) { return( & mObj[i] ); }

    u8  * bytes( idx i ) { return( reinterpret_cast<u8  *>(& mObj[i]) ); }
    u16 * uns16( idx i ) { return( reinterpret_cast<u16	*>(& mObj[i]) ); }
    u32 * uns32( idx i ) { return( reinterpret_cast<u32	*>(& mObj[i]) ); }
    u64 * uns64( idx i ) { return( reinterpret_cast<u64	*>(& mObj[i]) ); }
    i16 * int16( idx i ) { return( reinterpret_cast<i16	*>(& mObj[i]) ); }
	i32 * int32( idx i ) { return( reinterpret_cast<i32	*>(& mObj[i]) ); }
	i64 * int64( idx i ) { return( reinterpret_cast<i64 *>(& mObj[i]) ); }

  private:
	idx  	 objParse( u8 byte );  // next input byte -> true: complete

  protected:
    u16      mOffset;   // position in frame
    u16      mCrcRead;  // CRC from input data
    Crc      mCrc;		// updated on each byte received

  private:
    u8       mStatus;	// Status
    u8		 mErr;
    u8       mByteCnt; 	// common byte counter
    idx      mObjCnt;   // next free object index
    idx 	 mParsing;  // pointer to currently parsed object
    SmlObj   mObj[cMaxNofObj]; // storage of objects
    idx		 mElemCnt[cMaxListDepth];  // to stack the list elements counters

    enum Status {
    	EscBegin,	// 4x Byte::Escape -> Status::Begin
		Begin,		// 4x Byte::Begin  -> Status::Parse
		Parse,		// on Byte::Escape -> Status::EscEnd
		EscEnd,		// 4x Byte::Escape -> Status::End
		End			// 4x Bytes (CRC)  -> Status::EscBegin
	};

    void     objDump( idx o, u8 indent );

  private:
	static const char sTypechar[];
	static const u8   sTypePlus1;
	static const u8   sTypeInt;
	static const u8   sTypeInvalid;
};
