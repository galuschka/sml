#include "sml.h"
#include <string.h>	// memset()

// @fmt:off
const char Sml::sTypechar[] = { 'o', '?', '?', '?', 'b', 'i', 'u', 'L' };
const u8 Sml::sTypePlus1 =   { (1 << (Type::ByteStr  >> Byte::TypeShift))
                             | (1 << (Type::Boolean  >> Byte::TypeShift))
                             | (1 << (Type::Integer  >> Byte::TypeShift))
                             | (1 << (Type::Unsigned >> Byte::TypeShift)) };
const u8 Sml::sTypeInt =     { (1 << (Type::Integer  >> Byte::TypeShift))
                             | (1 << (Type::Unsigned >> Byte::TypeShift)) };
const u8 Sml::sTypeInvalid = { (1 << (Type::Invalid1 >> Byte::TypeShift))
                             | (1 << (Type::Invalid2 >> Byte::TypeShift))
                             | (1 << (Type::Invalid3 >> Byte::TypeShift)) };
// @fmt:on

Sml::Sml() :
        mOffset { 0 }, mCrcRead { 0 }, mCrc { }, mStatus { Status::EscBegin }, mErr {
                Err::NoError }, mByteCnt { 0 }, mObjCnt { 0 }, mParsing { 0 }
{
}

Sml::~Sml()
{
}

void Sml::parse( u8 byte )
{
    ++mOffset;

    switch (mStatus)
    {
        case Status::EscBegin:
            if (byte == Byte::Escape) {
                ++mByteCnt;
                return;
            }
            if ((byte == Byte::Begin) && (mByteCnt >= cMinNofEscBegin)) {
                mByteCnt = 1;
                mStatus = Status::Begin;
                return;
            }
            mByteCnt = 0;
            break;

        case Status::Begin:
            if (byte == Byte::Begin) {
                ++mByteCnt;
                if (mByteCnt == cNofBegin) {
                    mByteCnt = 0;
                    mStatus = Status::Parse;
                    start();
                }
                return;
            }
            mByteCnt = 0;
            if (byte == Byte::Escape)
                mByteCnt = 1;
            mStatus = Status::EscBegin;
            break;

        case Status::EscEnd:
            mCrc.add( byte );
            if (byte == Byte::Escape) {
                ++mByteCnt;
                return;
            }
            if (mByteCnt >= cNofEscEnd) {
                if (byte == Byte::End) {
                    mByteCnt = 1;
                    mStatus = Status::End;
                    return;
                }
                if (byte == Byte::Begin) {
                    mByteCnt = 1;
                    mStatus = Status::Begin;
                    return;
                }
            }
            mByteCnt = 0;
            // mStatus = Status::EscBegin;
            break;

        case Status::End:
            // mByteCnt == 1 ==> byte is number of fill bytes
            // mByteCnt == 2 ==> byte 1st CRC value
            // mByteCnt == 3 ==> byte 2nd CRC value
            switch (++mByteCnt)
            {
                case 2:
                    mCrc.add( byte );
                    break;
                case 3:
                    mCrcRead = byte;
                    break;
                case 4:
                    mCrcRead |= (u16) byte << 8;
                    if (mCrcRead != mCrc.get())
                        ready( Err::CrcError, byte );
                    else
                        ready( Err::NoError, byte );;
                    break;

                default:
                    break;
            }
            break;

        case Status::Parse:
            mCrc.add( byte );
            mParsing = objParse( byte );
            break;

        default:
            break;
    }
}

void Sml::start()
{
    mObjCnt = 0;
    mOffset = cNofEscBegin + cNofBegin - 1;
    mByteCnt = 0;
    mParsing = newObj( Obj::typesize( Type::List, 0 ), 0 );
    mErr = Err::NoError;

    mCrc.init();
    for (u8 i = 0; i < cNofEscBegin; ++i)
        mCrc.add( Byte::Escape );
    for (u8 i = 0; i < cNofBegin; ++i)
        mCrc.add( Byte::Begin );

    /*
     mRoot->mCount = 1;
     mRoot->mVal.list = new SmlObj( Type::List, mRoot );
     mParsing = mRoot->mVal.list;
     */
}

void Sml::timeout()
{
    ready( Err::Timeout, 0 );
}

idx Sml::ready( u8 err, u8 byte )
{
    mErr = err;
    if (mObjCnt) {
        fixup();  // optional to fix smart meter bug

        ++mErrCnt[ mErr < Err::Unknown ? mErr : Err::Unknown ];
        onReady( mErr, byte );
    }
    mStatus = Status::EscBegin;
    mObjCnt = 0;
    mByteCnt = byte == Byte::Escape ? 1 : 0;

    return (0);  // used to pass to return(ready(...))
}

idx Sml::newObj( u16 typeSize, idx parent )  // object constructor
{
    if (mObjCnt >= (cMaxNofObj - 1))
        return (0);

    const idx ret = mObjCnt++;
    ObjDef &p = intObjDef( ret );

    p.mParent = parent;
    p.mNext = 0;
    p.mTypeSize = typeSize;
    p.mVal = 0;

    if ((Obj::type( typeSize ) != Type::List) &&
        (Obj::size( typeSize ) > sizeof(idx))) {
        if (! (p.mVal = newData( Obj::size( typeSize ))))
            return (0);
    }

    return (ret);
}

idx Sml::newData( u16 size )
{
    const u8 nofObjs = (u8) ((size + sizeof(ObjDef) - 1) / sizeof(ObjDef));

    // todo: make it 64-bit savvy for 64-bit architecture:
    //       check size 8 because of scalar (not octet string)
    //       and align mObjCnt before assignment to ret

    if ((mObjCnt + nofObjs) >= cMaxNofObj)
        return (0);
    const idx ret = mObjCnt;
    mObjCnt += nofObjs;
    memset( intBytes( ret ), 0, nofObjs * sizeof(ObjDef) );
    return (ret);
}

idx Sml::objParse( u8 byte )
{
    static u16 s_typesize = 0;
    u16 typesize;

    ObjDef &p = intObjDef( mParsing );
    switch (Obj::type( p.mTypeSize ))
    {
        case Type::List:
            if (byte == Byte::MsgEnd)
                break;

            if (byte == Byte::Escape) {
                mByteCnt = 1;
                mStatus = Status::EscEnd;
                return 0;
            }

            if ((sTypeInvalid >> (byte >> Byte::TypeShift)) & 1)
                return (ready( Err::InvalidType, byte ));

            if (s_typesize) {  // next byte for typesize construction:
                s_typesize = Obj::typesize(
                                Obj::type( s_typesize ),
                               (Obj::size( s_typesize ) << Byte::TypeShift) | (byte & Byte::SizeMask) );
            }
            if (byte & Byte::FollowBit) {
                if (! s_typesize)  // first byte for typesize construction:
                    s_typesize = Obj::typesize( byte & Byte::TypeMask, byte & Byte::SizeMask );
                return mParsing;
            }
            if (s_typesize) {
                typesize = s_typesize;
                s_typesize = 0;
            } else
                typesize = Obj::typesize( byte & Byte::TypeMask, byte & Byte::SizeMask );

            if ((Obj::type( typesize ) != Type::List) && Obj::size( typesize ))
                --typesize;  // just length of lists do not contain type bytes itself

            {
                idx *link = &p.mVal;
                while (*link)
                    link = &intObjDef( *link ).mNext;
                idx const i = *link = newObj( typesize, mParsing );
                if (!i)
                    return (ready( Err::OutOfMemory, byte ));

                if ((Obj::type( typesize ) == Type::List) ||
                    (Obj::size( typesize ) != 0)) {
                    for (u8 depth = cMaxListDepth - 1; depth; --depth)
                        mElemCnt[depth] = mElemCnt[depth - 1];
                    mElemCnt[0] = mByteCnt;
                    mByteCnt = 0;
                    return (i);
                }
                // else: 01 -> zero length byte string -> elem. done
            }
            break;

        case Type::ByteStr:
            if (Obj::size( p.mTypeSize ) > sizeof(idx))
                intBytes( p.mVal )[mByteCnt] = byte;
            else
                ((u8 *) &p.mVal)[ mByteCnt ] = byte;
            break;

        default:
            // signed/unsigned: when signed and MSBit set: preset with ffff...
            if ((! mByteCnt) && (byte & 0x80) && (Obj::type( p.mTypeSize ) == Type::Integer))
            {
                const u16 size = Obj::size( p.mTypeSize );
                if (size > sizeof(idx)) {
                    u8 nofI32 = (u8) ((size + sizeof(u32) - 1) / sizeof(u32));
                    i32 *i32ptr = intI32ptr( p.mVal );
                    while (nofI32--)
                        *i32ptr++ = -1;
                } else
                    p.mVal = (idx) -1;
            }
            switch (Obj::size( p.mTypeSize ))
            {
                case 0:
                case 1:
                case 2:
                    p.mVal = (p.mVal << 8) | byte;
                    break;
                case 3:
                case 4:
                    *intU32ptr( p.mVal ) = (*intU32ptr( p.mVal ) << 8) | byte;
                    break;
                default:
                    *intU64ptr( p.mVal ) = (*intU64ptr( p.mVal ) << 8) | byte;
                    break;
            }
            break;
    }

    idx ret = mParsing;
    while (ret) {
        if (++mByteCnt < Obj::size( intObjDef( ret ).mTypeSize ))
            break;
        if (! Obj::size( intObjDef( ret ).mTypeSize ))
            break;
        ret = intObjDef( ret ).mParent;  // just lists can be parents --> ret is a list
        mByteCnt = mElemCnt[0];
        for (u8 depth = 0; depth < (cMaxListDepth - 1); ++depth)
            mElemCnt[depth] = mElemCnt[depth + 1];
    }
    return (ret);
}

const u8* Obj::bytes( u8 & len ) const
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

u8 Obj::getU8( bool & typematch ) const
{
    if (typesize() == typesize( Type::Unsigned, 1 )) {
        typematch = true;
        return (intU8());
    } else if (typesize() == typesize( Type::Integer, 1 )) {
        if (intI8() >= 0) {
            typematch = true;
            return intI8();
        }
    }
    typematch = false;
    return (0xff);
}
i8 Obj::getI8( bool & typematch ) const
{
    if (typesize() == typesize( Type::Integer, 1 )) {
        typematch = true;
        return (intI8());
    } else if (typesize() == typesize( Type::Unsigned, 1 )) {
        if (intI8() >= 0) {
            typematch = true;
            return intI8();
        }
    }
    typematch = false;
    return (0x80);
}

u16 Obj::getU16( bool & typematch ) const
{
    if (isType(Type::Unsigned)) {
        switch (size()) {
            case 1: typematch = true;
                    return intU8();
            case 2: typematch = true;
                    return intU16();
        }
    } else if (isType(Type::Integer)) {
        switch (size()) {
            case 1: if (intI8() >= 0) {
                        typematch = true;
                        return intU8();
                    }
                    break;
            case 2: if (intI16() >= 0) {
                        typematch = true;
                        return intU16();
                    }
                    break;
        }
    }
    typematch = false;
    return (0xffff);
}
i16 Obj::getI16( bool & typematch ) const
{
    if (isType(Type::Integer)) {
        switch (size()) {
            case 1: typematch = true;
                    return intI8();
            case 2: typematch = true;
                    return intI16();
        }
    } else if (isType(Type::Unsigned)) {
        switch (size()) {
            case 1: typematch = true;
                    return (i16) intU8();
            case 2: if (intI16() >= 0) {
                        typematch = true;
                        return intI16();
                    }
                    break;
        }
    }
    typematch = false;
    return (0x8000);
}

u32 Obj::getU32( bool & typematch ) const
{
    if (isType(Type::Unsigned)) {
        switch (size()) {
            case 1: typematch = true;
                    return intU8();
            case 2: typematch = true;
                    return intU16();
            case 3:
            case 4: typematch = true;
                    return intU32();
        }
    } else if (isType(Type::Integer)) {
        switch (size()) {
            case 1: if (intI8() >= 0) {
                        typematch = true;
                        return intU8();
                    }
                    break;
            case 2: if (intI16() >= 0) {
                        typematch = true;
                        return intU16();
                    }
                    break;
            case 3:
            case 4: if (intI32() >= 0) {
                        typematch = true;
                        return intU32();
                    }
                    break;
        }
    }
    typematch = false;
    return (0xffffffff);
}
i32 Obj::getI32( bool & typematch ) const
{
    if (isType(Type::Integer)) {
        switch (size()) {
            case 1: typematch = true;
                    return intI8();
            case 2: typematch = true;
                    return intI16();
            case 3:
            case 4: typematch = true;
                    return intI32();
        }
    } else if (isType(Type::Unsigned)) {
        switch (size()) {
            case 1: typematch = true;
                    return (i32) intU8();
            case 2: typematch = true;
                    return (i32) intU16();
            case 3:
            case 4: if (intI32() >= 0) {
                        typematch = true;
                        return intI32();
                    }
                    break;
        }
    }
    typematch = false;
    return (0x80000000);
}

u64 Obj::getU64( bool & typematch ) const
{
    if (isType(Type::Unsigned)) {
        typematch = true;
        switch (size()) {
            case 1: return intU8();
            case 2: return intU16();
            case 3:
            case 4: return intU32();
            default: return intU64();
        }
    } else if (isType(Type::Integer)) {
        switch (size()) {
            case 1: if (intI8() >= 0) {
                        typematch = true;
                        return intU8();
                    }
                    break;
            case 2: if (intI16() >= 0) {
                        typematch = true;
                        return intU16();
                    }
                    break;
            case 3:
            case 4: if (intI32() >= 0) {
                        typematch = true;
                        return intU32();
                    }
                    break;
            default: if (intI64() >= 0) {
                        typematch = true;
                        return intU64();
                    }
                    break;
        }
    }
    typematch = false;
    return (0xffffffffffffffff);
}
i64 Obj::getI64( bool & typematch ) const
{
    if (isType(Type::Integer)) {
        typematch = true;
        switch (size()) {
            case 1: return intI8();
            case 2: return intI16();
            case 3:
            case 4: return intI32();
            default: return intI64();

        }
    } else if (isType(Type::Unsigned)) {
        switch (size()) {
            case 1: typematch = true;
                    return (i64) intU8();
            case 2: typematch = true;
                    return (i64) intU16();
            case 3:
            case 4: typematch = true;
                    return (i64) intU32();

            default: if (intI64() >= 0) {
                        typematch = true;
                        return intI64();
                    }
                    break;
        }
    }
    typematch = false;
    return (0x8000000000000000);
}
