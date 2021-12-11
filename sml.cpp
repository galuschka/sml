#include "sml.h"
#include <stdexcept>

Sml::Sml() : mStatus{Status::EscBegin}, mEscCnt{0}, mRoot{nullptr}, mParsing{nullptr}
{
}

Sml::~Sml()
{
}

void Sml::parse( u8 byte )
{
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
                case 3: mCrcRead = (u16) byte << 8; break;
                case 4: mCrcRead |= byte;
                    // if (mCrcRead == mCrcCalc)
                        show();
                    mStatus = Status::EscBegin;
            }
            return;

        case (u8) Status::Parse:
            if (mParsing)
                mParsing = mParsing->parse( byte );
            if (! mParsing) {
                mStatus = Status::EscEnd;
                if (byte == (u8) Byte::Escape)
                    mEscCnt = 1;
            }
            return;
    }
}

void Sml::start()
{
    if (mRoot)
        delete mRoot;
    mRoot = new SmlObj{};
    mRoot->mCount = 1;
    mRoot->mVal.list = new SmlObj( (u8) Type::List, mRoot );
    mParsing = mRoot->mVal.list;
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
        mSize( 1 ),
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

SmlObj * SmlObj::parse( u8 byte )
{
    switch (mType)
    {
        case Type::List:
            if (byte == (u8) Byte::MsgEnd)
                break;
            if (byte == (u8) Byte::Escape)
                return nullptr;
            
            switch (byte)
            {
                case (u8) Type::Invalid1:
                case (u8) Type::Invalid2:
                case (u8) Type::Invalid3:
                    throw std::runtime_error("invalid type byte");
            }
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
    printf( "%*s", indent * 4, "" );
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
            puts( "list:" );
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
                case 1:  printf(   "%d = 0x%0*x"   "\n", mVal._i8 , mSize*2, mVal._u8  ); break;
                case 2:  printf(   "%d = 0x%0*x"   "\n", mVal._i16, mSize*2, mVal._u16 ); break;
                case 3:
                case 4:  printf(  "%ld = 0x%0*lx"  "\n",      (long) mVal._i32, mSize*2,
                                                     (unsigned long) mVal._u32 ); break;
                default: printf( "%lld = 0x%0*llx" "\n", (long long) mVal._i64, mSize*2,
                                                (unsigned long long) mVal._u64 ); break;
            }
            break;
        case Type::Unsigned:
            switch (mSize) {
                case 0:
                case 1:  printf(   "%u = 0x%0*x"   "\n", mVal._u8 , mSize*2, mVal._u8  ); break;
                case 2:  printf(   "%u = 0x%0*x"   "\n", mVal._u16, mSize*2, mVal._u16 ); break;
                case 3:
                case 4:  printf(  "%lu = 0x%0*lx"  "\n", (unsigned long)      mVal._u32, mSize*2,
                                                         (unsigned long)      mVal._u32 ); break;
                default: printf( "%llu = 0x%0*llx" "\n", (unsigned long long) mVal._u64, mSize*2,
                                                         (unsigned long long) mVal._u64 ); break;
            }
            break;
        default:
            printf(   "unknown type %x"    "\n", (u8) mType  );
            break;
    }
}
