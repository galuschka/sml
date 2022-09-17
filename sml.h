#pragma once

#include <stdint.h>
#include "crc16.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef u16 idx;        // number of objects for extended output is close to 256

const idx cMaxNofObj = 0x120;   // <- adjust according your needs
const u8 cMaxListDepth = 10;  // while parsing nested list, we have to save old element counter
const u8 cMinNofEscBegin = 3;  // ok, when Byte::Begin after 3 Byte::Escape
const u8 cNofEscBegin = 4;    // usual number of Byte::Escape
const u8 cNofBegin = 4;       // number of Byte::Begin
const u8 cNofEscEnd = 4;      // number of Byte::Escape before Byte::End

namespace Byte {
enum Byte
{
    MsgEnd = 0,
    Skip = 1,  // optional
    Begin = 1,
    End = 0x1a,
    Escape = 0x1b,
    SizeMask = 0x0f,
    TypeMask = 0x70,
    FollowBit = 0x80,  // see example below
    TypeShift = 4,  // type >> 4 is basic type value

// FollowBit example: input data: f1 04
//  f1 & 80 == 80: follow bit set -> next byte will be added to type-length
//  f1 & 70 == 70: ByteType::List (note: when type boolean, type is extended - not length)
//  f1 & 0f == 01: first 4 length bits: 1 (will be shifted by 4 for each following)
//  04 & 80 ==  0: no more following length bits
//  04 & 0f ==  4: next 4 length bits: 4 -> ((1 << 4)|4) = 0x14
// f1 04 ==> List of 0x14 (decimal 20) elements
};
}

namespace Type {
enum Type
{
    ByteStr = 0,
    Invalid1 = 0x10,
    Invalid2 = 0x20,
    Invalid3 = 0x30,
    Boolean = 0x40,
    Integer = 0x50,
    Unsigned = 0x60,
    List = 0x70,
};
}

namespace Err {
enum Err
{
    NoError = 0,
    OutOfMemory,	// cMaxNofObj too small
    InvalidType,
    CrcError,

    Unknown         // number of error types and index for invalid err values
};
}

class Sml;

class ObjDef
{
    public:
        idx mParent;   // parent list (the list containing this)
        idx mNext;     // next element in same list (nil when last)
        u16 mTypeSize; // object type and size (type nibble shifted by 8)
        idx mVal;      // u8 or u16 value or index to list or index to value
};

class Sml
{
    public:
        Sml();
        virtual ~Sml() = 0;

        void parse( u8 byte );  // parse next input byte
        void dump( const char * header = 0 );  // dump the struct

        virtual void onReady( u8 err, u8 byte ) = 0;  // method called on parsing complete/abort

// @fmt:off
        const ObjDef& extObjDef( idx i ) const { return (mObjDef[i]); }

        u8 const* extBytes( idx i ) const { return (reinterpret_cast<u8 const *>( &mObjDef[i] )); }
        u16 extU16( idx i ) const { return (*reinterpret_cast<u16 const *>( &mObjDef[i] )); }
        u32 extU32( idx i ) const { return (*reinterpret_cast<u32 const *>( &mObjDef[i] )); }
        u64 extU64( idx i ) const { return (*reinterpret_cast<u64 const *>( &mObjDef[i] )); }
        i16 extI16( idx i ) const { return (*reinterpret_cast<i16 const *>( &mObjDef[i] )); }
        i32 extI32( idx i ) const { return (*reinterpret_cast<i32 const *>( &mObjDef[i] )); }
        i64 extI64( idx i ) const { return (*reinterpret_cast<i64 const *>( &mObjDef[i] )); }
// @fmt:on

        idx objCnt() const
        {
            return mObjCnt;
        }

    private:
        void start();           // start parsing objects (behind esc begin)
        idx abort( u8 err );    // set to abort parsing (error or end detection)

        idx newObj( u16 typesize, idx parent );
        idx newData( u16 size );

// @fmt:off
        ObjDef& intObjDef( idx i ) { return (mObjDef[i]); }

        u8*  intBytes( idx i ) { return (reinterpret_cast<u8 *>( &mObjDef[i] )); }
        u16* intU16( idx i ) { return (reinterpret_cast<u16 *>( &mObjDef[i] )); }
        u32* intU32( idx i ) { return (reinterpret_cast<u32 *>( &mObjDef[i] )); }
        u64* intU64( idx i ) { return (reinterpret_cast<u64 *>( &mObjDef[i] )); }
        i16* intI16( idx i ) { return (reinterpret_cast<i16 *>( &mObjDef[i] )); }
        i32* intI32( idx i ) { return (reinterpret_cast<i32 *>( &mObjDef[i] )); }
        i64* intI64( idx i ) { return (reinterpret_cast<i64 *>( &mObjDef[i] )); }
// @fmt:on

        idx objParse( u8 byte );  // next input byte -> true: complete

    protected:
        u16 mOffset;    // position in frame
        u16 mCrcRead;   // CRC from input data
        Crc mCrc;	    // updated on each byte received

    private:
        u8 mStatus;                   // Status
        u8 mErr;
        u8 mByteCnt;                  // common byte counter
        idx mObjCnt;                  // next free object index
        idx mParsing;                 // pointer to currently parsed object
        // 4-byte alignment is fine, since either u64 is in SW anyway
        // (on 32-bit architecture) or we have to check alignment in newData()
        alignas(4) ObjDef mObjDef[cMaxNofObj];   // storage of objects
        idx mElemCnt[cMaxListDepth];  // to stack the list elements counters

        enum Status
        {
            EscBegin,	// 4x Byte::Escape -> Status::Begin
            Begin,		// 4x Byte::Begin  -> Status::Parse
            Parse,		// on Byte::Escape -> Status::EscEnd
            EscEnd,		// 4x Byte::Escape -> Status::End
            End			// 4x Bytes (CRC)  -> Status::EscBegin
        };

        void objDump( idx o, u8 indent );

    private:
        static const char sTypechar[];
        static const u8 sTypePlus1;
        static const u8 sTypeInt;
        static const u8 sTypeInvalid;
};

class Obj
{
        Obj() = delete;
    public:
        ~Obj() = default;

        Obj( Sml & sml, idx start ) :
                mSml { sml }, mIdx { 0 }  // start-th element of root
        {
            mIdx = mSml.extObjDef( 0 ).mVal;
            while (start--) {
                mIdx = mSml.extObjDef( mIdx ).mNext;
                if (mIdx >= mSml.objCnt()) {
                    mIdx = 0;  // == no such object
                    break;
                }
            }
        }

        Obj( Obj & obj, idx i ) :
                mSml { obj.mSml }, mIdx { obj.mIdx }  // i-th element of list
        {
            if (!isType( Type::List )) {
                mIdx = 0;  // == no such object
                return;
            }
            mIdx = mSml.extObjDef( mIdx ).mVal;
            while (i--) {
                mIdx = mSml.extObjDef( mIdx ).mNext;
                if (mIdx >= mSml.objCnt()) {
                    mIdx = 0;  // == no such object
                    break;
                }
            }
        }

        const ObjDef& objDef() const
        {
            return (mSml.extObjDef( mIdx ));
        }
        u16 typesize() const
        {
            if (!mIdx || (mIdx >= mSml.objCnt()))
                return (0);
            return (objDef().mTypeSize);
        }

        static u16 typesize( u8 type, u16 size )
        {
            return (((u16) type) << 8) | size;
        }
        static u8 type( u16 typesize )
        {
            return ((typesize >> 8) & Byte::TypeMask);
        }
        static u16 size( u16 typesize )
        {
            return (typesize & ((Byte::SizeMask << 8) | 0xff));
        }

        u8 type() const
        {
            return (type( typesize() ));
        }
        u16 size() const
        {
            return (size( typesize() ));
        }

        bool isType( u8 cmptype ) const
        {
            if (!mIdx || (mIdx >= mSml.objCnt()))
                return (false);
            return (type() == cmptype);
        }
        bool isTypeSize( u8 type, u8 size ) const
        {
            if (!mIdx || (mIdx >= mSml.objCnt()))
                return (false);
            return (typesize() == typesize( type, size ));
        }

        const u8* bytes( u8 & len ) const
        {
            if (!isType( Type::ByteStr )) {
                len = 0;
                return (nullptr);
            }
            len = size();
            if (len <= sizeof(idx))
                return (u8*)(&objDef().mVal);
            return (mSml.extBytes( objDef().mVal ));
        }

        u8 getU8( bool & typematch ) const
        {
            if (typesize() == typesize( Type::Unsigned, 1 )) {
                typematch = true;
                return (intU8());
            }
            typematch = false;
            return (0xff);
        }
        u16 getU16( bool & typematch ) const
        {
            if (typesize() == typesize( Type::Unsigned, 2 )) {
                typematch = true;
                return (intU16());
            }
            typematch = false;
            return (0xffff);
        }
        u32 getU32( bool & typematch ) const
        {
            if (typesize() == typesize( Type::Unsigned, 4 )) {
                typematch = true;
                return (intU32());
            }
            typematch = false;
            return (0xffffffff);
        }
        u64 getU64( bool & typematch ) const
        {
            if (typesize() == typesize( Type::Unsigned, 8 )) {
                typematch = true;
                return (intU64());
            }
            typematch = false;
            return (0xffffffffffffffff);
        }
        i8 getI8( bool & typematch ) const
        {
            if (typesize() == typesize( Type::Integer, 1 )) {
                typematch = true;
                return (intI8());
            }
            typematch = false;
            return (-1);
        }
        i16 getI16( bool & typematch ) const
        {
            if (typesize() == typesize( Type::Integer, 2 )) {
                typematch = true;
                return (intI16());
            }
            typematch = false;
            return (-1);
        }
        i32 getI32( bool & typematch ) const
        {
            if (typesize() == typesize( Type::Integer, 4 )) {
                typematch = true;
                return (intI32());
            }
            typematch = false;
            return (-1);
        }
        i64 getI64( bool & typematch ) const
        {
            if (typesize() == typesize( Type::Integer, 8 )) {
                typematch = true;
                return (intI64());
            }
            typematch = false;
            return (-1);
        }

        void dump( const char * name ) const;

    private:
// @fmt:off
        u8  intU8()  const { return (u8)  objDef().mVal; }
        u16 intU16() const { return (u16) objDef().mVal; }
        u32 intU32() const { return (mSml.extU32( objDef().mVal )); }
        u64 intU64() const { return (mSml.extU64( objDef().mVal )); }

        i8  intI8()  const { return (i8)  objDef().mVal; }
        i16 intI16() const { return (i16) objDef().mVal; }
        i32 intI32() const { return (mSml.extI32( objDef().mVal )); }
        i64 intI64() const { return (mSml.extI64( objDef().mVal )); }
// @fmt:on

        const Sml &mSml;
        idx mIdx;

        // friend class Sml;
};
