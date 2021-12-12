#include "sml.h"
#include <stdio.h>

const char SmlObj::sTypechar[]  = { 'o','?','?','?','b','i','u','L' };
const u8   SmlObj::sTypePlus1   = { (1 << ((u8) Type::ByteStr  >> (u8) Byte::TypeShift))
                                  | (1 << ((u8) Type::Boolean  >> (u8) Byte::TypeShift))
                                  | (1 << ((u8) Type::Integer  >> (u8) Byte::TypeShift))
                                  | (1 << ((u8) Type::Unsigned >> (u8) Byte::TypeShift))
                                  };
const u8   SmlObj::sTypeInt     = { (1 << ((u8) Type::Integer  >> (u8) Byte::TypeShift))                                 
                                  | (1 << ((u8) Type::Unsigned >> (u8) Byte::TypeShift))
                                  };
const u8   SmlObj::sTypeInvalid = { (1 << ((u8) Type::Invalid1 >> (u8) Byte::TypeShift))                                 
                                  | (1 << ((u8) Type::Invalid2 >> (u8) Byte::TypeShift))
                                  | (1 << ((u8) Type::Invalid3 >> (u8) Byte::TypeShift))
                                  };

Sml::Sml() : mCrc{},
             mStatus{Status::EscBegin},
             mEscCnt{0},
             mRoot{nullptr},
             mParsing{nullptr}
{
}

Sml::~Sml()
{
}

void Sml::parse( u8 byte )
{
    ++mOffset;

    switch ((u8) mStatus)
    {
        case (u8) Status::EscBegin:
            if (byte == (u8) Byte::Escape) {
                ++mEscCnt;
                return;
            }
            if ((byte == (u8) Byte::Begin) && (mEscCnt >= 4)) {
                mEscCnt = 1;
                mStatus = Status::Begin;
                return;
            }
            mEscCnt = 0;
            return;

        case (u8) Status::Begin:
            if (byte == (u8) Byte::Begin) {
                ++mEscCnt;
                if (mEscCnt == 4) {
                    mEscCnt = 0;
                    mStatus = Status::Parse;
                    start();
                }
                return;
            }
            mEscCnt = 0;
            if (byte == (u8) Byte::Escape)
                mEscCnt = 1;
            mStatus = Status::EscBegin;
            return;

        case (u8) Status::EscEnd:
            mCrc.add(byte);
            if (byte == (u8) Byte::Escape) {
                ++mEscCnt;
                return;
            }
            if (mEscCnt >= 4) {
                if (byte == (u8) Byte::End) {
                    mEscCnt = 1;
                    mStatus = Status::End;
                    return;
                }
                if (byte == (u8) Byte::Begin) {
                    mEscCnt = 1;
                    mStatus = Status::Begin;
                    return;
                }
            }
            mEscCnt = 0;
            // mStatus = Status::EscBegin;
            return;

        case (u8) Status::End:
            // mEscCnt == 1 ==> byte is number of fill bytes
            // mEscCnt == 2 ==> byte 1st CRC value
            // mEscCnt == 3 ==> byte 2nd CRC value
            switch (++mEscCnt)
            {
                case 2:
                    mCrc.add(byte);
                    break;
                case 3:
                    mCrcRead = byte;
                    break;
                case 4:
                    mCrcRead |= (u16) byte << 8;
                    onReady( mCrcRead != mCrc.get()
                                ? Err::CrcError
                                : Err::NoError, byte );
                    mStatus = Status::EscBegin;
                    break;
            }
            return;

        case (u8) Status::Parse:
            mCrc.add(byte);
            if (mParsing)
                mParsing = mParsing->parse( byte, *this );
            if (! mParsing) {
                if (mStatus == Status::EscBegin) {
                    onReady( Err::InvalidType, byte );
                } else {
                    mStatus = Status::EscEnd;
                    if (byte == (u8) Byte::Escape)
                        mEscCnt = 1;
                }
            }
            return;
    }
}

void Sml::start()
{
    if (mRoot)
        delete mRoot;
    mRoot = new SmlObj{};
    mParsing = mRoot;
    mOffset = 7;
    
    mCrc.init();
    mCrc.add( (u8) Byte::Escape );
    mCrc.add( (u8) Byte::Escape );
    mCrc.add( (u8) Byte::Escape );
    mCrc.add( (u8) Byte::Escape );
    mCrc.add( (u8) Byte::Begin );
    mCrc.add( (u8) Byte::Begin );
    mCrc.add( (u8) Byte::Begin );
    mCrc.add( (u8) Byte::Begin );

/*
    mRoot->mCount = 1;
    mRoot->mVal.list = new SmlObj( (u8) Type::List, mRoot );
    mParsing = mRoot->mVal.list;
*/
}

void Sml::abort()
{
    mStatus = Status::EscBegin;
}

void Sml::show()
{
    if (mRoot) {
        mRoot->showobj( (u8) 0 );
        delete mRoot;
        mRoot = nullptr;
    }
}

SmlObj::SmlObj()  // constructor for root only
      : mVal(),
        mType( Type::List ),
        mSize( 0 ),
        mCount( 0 ),
        mNext( nullptr ),
        mParent( nullptr )
{
}

SmlObj::SmlObj( u8 byte, SmlObj * parent )  // constructor for all other
      : mVal(),
        mType( (Type) (byte & (u8) Byte::TypeMask) ),
        mSize(         byte & (u8) Byte::SizeMask ),
        mCount( 0 ),
        mNext( nullptr ),
        mParent( parent )
{
    if (mSize && (mType != Type::List))
        --mSize;

    switch (mType)
    {
        case Type::ByteStr:
            if (mSize)
                mVal.byteStr = new u8[mSize];
            break;
/*
        case Type::Invalid1:
        case Type::Invalid2:
        case Type::Invalid3:
            throw std::runtime_error("invalid type");
*/
        default:
            break;
    }
}

SmlObj::~SmlObj()
{
    switch ((u8) mType)
    {
        case (u8) Type::ByteStr:
            if (mSize)
                delete[] mVal.byteStr;
            break;

        case (u8) Type::List:
            SmlObj * obj = mVal.list;
            mVal.list = nullptr;
            while (obj) {
                SmlObj * next = obj->mNext;
                delete obj;
                obj = next;
            }
            break;
    }
}

SmlObj * SmlObj::parse( u8 byte, Sml & sml )
{
    switch (mType)
    {
        case Type::List:
            if (byte == (u8) Byte::MsgEnd)
                break;
            if (byte == (u8) Byte::Escape)
                return nullptr;
            if ((sTypeInvalid >> (byte >> (u8) Byte::TypeShift)) & 1) {
                sml.abort();
                return nullptr;
            }
            /*
            switch (byte)
            {
                case (u8) Type::Invalid1:
                case (u8) Type::Invalid2:
                case (u8) Type::Invalid3:
                    throw std::runtime_error("invalid type byte");
            }
            */
            {
                SmlObj ** link = & mVal.list;
                while (*link) link = & (*link)->mNext;
                *link = new SmlObj( byte, this );
                if ((*link)->mSize)
                    return *link;
                // else: 01 -> zero length byte string -> elem. done
            }
            break;

        case Type::ByteStr:
            mVal.byteStr[mCount] = byte;
            break;

        default:
            mVal._u64 <<= 8;
            mVal._u64 |= byte;
            break;
    }

    SmlObj * ret = this;
    while (ret) {
        if (++ret->mCount < ret->mSize)
            break;
        if (! ret->mSize)
            break;
        ret = ret->mParent;  // just lists can be parents --> ret is a list
    }
    return ret;
}

void SmlObj::showobj( u8 indent )
{
    printf( "%*s%c%d-%02x: ", indent * 4, "",
                sTypechar[(u8) mType >> (u8) Byte::TypeShift],
                mSize * ((((sTypeInt >> ((u8) mType >> (u8) Byte::TypeShift)) & 1) * 7) + 1),
                (u8) mType | (mSize + ((sTypePlus1 >> ((u8) mType >> (u8) Byte::TypeShift)) & 1)) );
    u8 i;
    SmlObj * elem;

    switch (mType)
    {
        case Type::ByteStr:
            for (i = 0; i < mSize; ++i)
                printf( "%02x ", mVal.byteStr[i] );
            fputs( "(\"", stdout );
            for (i = 0; i < mSize; ++i)
                if ((mVal.byteStr[i] >= 0x20) && (mVal.byteStr[i] < 0x7f))
                    putchar( mVal.byteStr[i] );
                else
                    putchar( '.' );
            puts( "\")" );
            break;

        case Type::List:
            // puts( "list of %d elements", mSize );
            puts( "" );
            i = 0;
            for (elem = mVal.list; elem; elem = elem->mNext) {
                printf( "%*s %d/%d:\n", indent * 4, "", ++i, mSize );
                elem->showobj( indent + 1 );
            }
            break;

        case Type::Boolean: printf( "%s\n", mVal._u8 ? "true" : "false" ); break;
        case Type::Integer:
            switch (mSize) {
                case 0:
                case 1:  printf( "0x%0*x"   " = %d"   "\n", mSize*2, mVal._u8,  mVal._i8  ); break;
                case 2:  printf( "0x%0*x"   " = %d"   "\n", mSize*2, mVal._u16, mVal._i16 ); break;
                case 3:
                case 4:  printf( "0x%0*lx"  " = %ld"  "\n", mSize*2,
                                                     (unsigned long) mVal._u32,
                                                              (long) mVal._i32 ); break;
                default: printf( "0x%0*llx" " = %lld" "\n", mSize*2,
                                                (unsigned long long) mVal._u64,
                                                         (long long) mVal._i64 ); break;
            }
            break;
        case Type::Unsigned:
            switch (mSize) {
                case 0:
                case 1:  printf( "0x%0*x"   " = %u"   "\n", mSize*2, mVal._u8,  mVal._u8  ); break;
                case 2:  printf( "0x%0*x"   " = %u"   "\n", mSize*2, mVal._u16, mVal._u16 ); break;
                case 3:
                case 4:  printf( "0x%0*lx"  " = %lu"  "\n", mSize*2,
                                                     (unsigned long) mVal._u32,
                                                     (unsigned long) mVal._i32 ); break;
                default: printf( "0x%0*llx" " = %llu" "\n", mSize*2,
                                                (unsigned long long) mVal._u64,
                                                (unsigned long long) mVal._i64 ); break;
            }
            break;
        default:
            printf(   "unknown type %x"    "\n", (u8) mType  );
            break;
    }
}
