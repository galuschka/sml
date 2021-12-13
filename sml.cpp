#include "sml.h"
#include <stdio.h>
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
                    onReady(
                            mCrcRead != mCrc.get() ?
                                    Err::CrcError : Err::NoError,
                            byte );
                    mStatus = Status::EscBegin;
                    break;

                default:
                    break;
            }
            break;

        case Status::Parse:
            mCrc.add( byte );
            mParsing = objParse( byte );

            if (!mParsing && (mStatus != Status::Parse)) {
                if (mStatus == Status::EscBegin) {
                    onReady( mErr, byte );
                    mByteCnt = 0;
                }
            }
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
    mParsing = newObj( Type::List, 0 );
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

idx Sml::abort( u8 err )
{
    mErr = err;
    if (err == Err::NoError) {
        mByteCnt = 1;
        mStatus = Status::EscEnd;
    } else {
        mByteCnt = 0;
        mStatus = Status::EscBegin;
    }
    return (0);
}

void Sml::dump()
{
    if (mObjCnt) {
        printf( "%d objects:\n", mObjCnt );
        objDump( 0, 0 );
        mObjCnt = 0;
    }
}

idx Sml::newObj( u8 byte, idx parent )  // object constructor
{
    if (mObjCnt >= (cMaxNofObj - 1))
        return (0);

    const idx ret = mObjCnt++;
    SmlObj *p = obj( ret );

    if (((byte & Byte::TypeMask) != Type::List) && (byte & Byte::SizeMask))
        --byte;

    p->mParent = parent;
    p->mNext = 0;
    p->mTypeSize = byte;
    p->mVal = 0;

    if (((p->mTypeSize & Byte::TypeMask) != Type::List)
            && ((p->mTypeSize & Byte::SizeMask) > 1)) {
        if (!(p->mVal = newData( p->mTypeSize & Byte::SizeMask )))
            return (0);
    }

    return (ret);
}

idx Sml::newData( u8 size )
{
    u8 effsize = size;
    switch (size)
    {
        case 1:
            effsize = sizeof(u8);
            break;
        case 2:
            effsize = sizeof(u16);
            break;
        case 4:
            effsize = sizeof(u32);
            break;
        case 8:
            effsize = sizeof(u64);
            break;
        default:
            break;
    }
    const u8 nofObjs = (effsize + sizeof(SmlObj) - 1) / sizeof(SmlObj);

    if (mObjCnt >= (cMaxNofObj - nofObjs))
        return (0);
    const idx ret = mObjCnt;
    mObjCnt += nofObjs;
    memset( bytes( ret ), 0, effsize );
    return (ret);
}

idx Sml::objParse( u8 byte )
{
    SmlObj *p = obj( mParsing );
    switch (p->mTypeSize & Byte::TypeMask)
    {
        case Type::List:
            if (byte == Byte::MsgEnd)
                break;

            if (byte == Byte::Escape)
                return (abort( Err::NoError ));

            if ((sTypeInvalid >> (byte >> Byte::TypeShift)) & 1)
                return (abort( Err::InvalidType ));

            {
                idx *link = &p->mVal;
                while (*link)
                    link = &obj( *link )->mNext;
                idx i = *link = newObj( byte, mParsing );
                if (!i)
                    return (abort( Err::OutOfMemory ));

                if (((byte & Byte::TypeMask) == Type::List)
                        || (obj( i )->mTypeSize & Byte::SizeMask)) {
                    for (u8 depth = cMaxListDepth - 1; depth; --depth)
                        mElemCnt[depth] = mElemCnt[depth - 1];
                    mElemCnt[0] = mByteCnt;
                    mByteCnt = 0;
                    return (*link);
                }
                // else: 01 -> zero length byte string -> elem. done
            }
            break;

        case Type::ByteStr:
            if ((p->mTypeSize & Byte::SizeMask) > 1)
                bytes( p->mVal )[mByteCnt] = byte;
            else
                p->mVal = byte;
            break;

        default:
            switch (p->mTypeSize & Byte::SizeMask)
            {
                case 0:
                case 1:
                    p->mVal = byte;
                    break;
                case 2:
                    *uns16( p->mVal ) = (*uns16( p->mVal ) << 8) | byte;
                    break;
                case 3:
                case 4:
                    *uns32( p->mVal ) = (*uns32( p->mVal ) << 8) | byte;
                    break;
                default:
                    *uns64( p->mVal ) = (*uns64( p->mVal ) << 8) | byte;
                    break;
            }
            break;
    }

    idx ret = mParsing;
    while (ret) {
        if (++mByteCnt < (obj( ret )->mTypeSize & Byte::SizeMask))
            break;
        if (!(obj( ret )->mTypeSize & Byte::SizeMask))
            break;
        ret = obj( ret )->mParent;  // just lists can be parents --> ret is a list
        mByteCnt = mElemCnt[0];
        for (u8 depth = 0; depth < (cMaxListDepth - 1); ++depth)
            mElemCnt[depth] = mElemCnt[depth + 1];
    }
    return (ret);
}

void Sml::objDump( idx o, u8 indent )
{
    SmlObj *const p = obj( o );
    u8 const size = p->mTypeSize & Byte::SizeMask;
    u8 const type = p->mTypeSize & Byte::TypeMask;

    printf( "%*s%c%d-%02x: ", indent * 4, "",
            sTypechar[type >> Byte::TypeShift],
            size * ((((sTypeInt >> (type >> Byte::TypeShift)) & 1) * 7) + 1),
            p->mTypeSize + ((sTypePlus1 >> (type >> Byte::TypeShift)) & 1) );
    u8 i;
    idx elem;
    u8 *byteStr;

    switch (p->mTypeSize & Byte::TypeMask)
    {
        case Type::ByteStr:
            if (size > 1)
                byteStr = bytes( p->mVal );
            else
                byteStr = &p->mVal;

            for (i = 0; i < size; ++i)
                printf( "%02x ", byteStr[i] );
            fputs( "\"", stdout );
            for (i = 0; i < size; ++i)
                if ((byteStr[i] >= 0x20) && (byteStr[i] < 0x7f))
                    putchar( byteStr[i] );
                else
                    putchar( '.' );
            puts( "\"" );
            break;

        case Type::List:
            // puts( "list of %d elements", mSize );
            puts( "" );
            i = 0;
            for (elem = p->mVal; elem; elem = obj( elem )->mNext) {
                printf( "%*s %d/%d:\n", indent * 4, "", ++i, size );
                objDump( elem, indent + 1 );
            }
            break;

        case Type::Boolean:
            printf( "%d = %s\n", p->mVal, p->mVal ? "true" : "false" );
            break;

        case Type::Integer:
            switch (size)
            {
                case 0:
                case 1:
                    printf( "0x%0*x = %d\n", size * 2, p->mVal, (i8) p->mVal );
                    break;
                case 2:
                    printf( "0x%0*x = %d\n", size * 2, *uns16( p->mVal ),
                            *int16( p->mVal ) );
                    break;
                case 3:
                case 4:
                    printf( "0x%0*x = %d\n", size * 2, *uns32( p->mVal ),
                            *int32( p->mVal ) );
                    break;
                default:
                    printf( "0x%0*lx = %ld\n", size * 2, *uns64( p->mVal ),
                            *int64( p->mVal ) );
                    break;
            }
            break;

        case Type::Unsigned:
            switch (size)
            {
                case 0:
                case 1:
                    printf( "0x%0*x = %u\n", size * 2, p->mVal, p->mVal );
                    break;
                case 2:
                    printf( "0x%0*x = %u\n", size * 2, *uns16( p->mVal ),
                            *uns16( p->mVal ) );
                    break;
                case 3:
                case 4:
                    printf( "0x%0*x = %u\n", size * 2, *uns32( p->mVal ),
                            *uns32( p->mVal ) );
                    break;
                default:
                    printf( "0x%0*lx = %lu\n", size * 2, *uns64( p->mVal ),
                            *uns64( p->mVal ) );
                    break;
            }
            break;

        default:
            puts( "?" );
            break;
    }
}
