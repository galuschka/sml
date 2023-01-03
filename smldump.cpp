#include "sml.h"

#include <stdio.h>

class SmlDump : public Sml
{
        virtual void onReady( u8 err, u8 byte ) = 0;

        void dump( const char * name = nullptr );
        void objDump( idx o, u8 indent );

    public:
        static SmlDump & Instance();
};

void Obj::dump( const char * name ) const
{
    printf( "%s: mIdx: %d, next: %d, type %02x, size %d, mVal: %d\n",
        name, mIdx, objDef().mNext, type(), size(), objDef().mVal );
}

void SmlDump::dump( const char * header )
{
    if (objCnt()) {
        if (header) {
            printf( "\n%s:\n", header );
        }
        printf( "%d objects:\n", objCnt() );
        objDump( 0, 0 );
    }

    if (header) {
        const u32 * errCnt = getErrCntArray();
        printf( "valid packets: %8u\n",   errCnt[Err::NoError] );
        printf( "out of memory: %8u%s\n", errCnt[Err::OutOfMemory],
                                          errCnt[Err::OutOfMemory]
                                    ? " -> increase cMaxNofObj in sml.h!" : "" );
        printf( "type errors:   %8u\n",   errCnt[Err::InvalidType] );
        printf( "CRC errors:    %8u\n",   errCnt[Err::CrcError] );
        printf( "timeout errors:%8u\n",   errCnt[Err::Timeout] );
        printf( "unknown errors:%8u\n",   errCnt[Err::Unknown] );
    }
}

void SmlDump::objDump( idx o, u8 indent )
{
    ObjDef &p = intObjDef( o );
    u8  const type = (p.mTypeSize >> 8) & Byte::TypeMask;
    u16 const size = p.mTypeSize & ((Byte::SizeMask << 8) | 0xff);

    printf( "%*s%c%d", indent * 4, "",
            sTypechar[type >> Byte::TypeShift],
            size * ((((sTypeInt >> (type >> Byte::TypeShift)) & 1) * 7) + 1) );
    u16 codingSize = size + ((sTypePlus1 >> (type >> Byte::TypeShift)) & 1);
    if (codingSize > Byte::SizeMask) {
        u8 byte[4];
        u8 * bp = &byte[3];
        *bp = codingSize & Byte::SizeMask;
        codingSize >>= Byte::TypeShift;
        while (codingSize > Byte::SizeMask) {
            *--bp = 0x80 | (codingSize & Byte::SizeMask);
            codingSize >>= Byte::TypeShift;
        }
        *--bp = 0x80 | type | codingSize;
        do
            printf( "-%02x", *bp );
        while (++bp < &byte[4]);
    }
    else
        printf( "-%02x", type | codingSize );
    printf( ": " );

    u8 i;
    idx elem;
    u8 *byteStr;

    switch (Obj::type( p.mTypeSize ))
    {
        case Type::ByteStr:
            if (size > sizeof(idx))
                byteStr = intBytes( p.mVal );
            else
                byteStr = (u8 *) &p.mVal;

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
            for (elem = p.mVal; elem; elem = intObjDef( elem ).mNext) {
                printf( "%*s %d/%d:\n", indent * 4, "", ++i, size );
                objDump( elem, indent + 1 );
            }
            break;

        case Type::Boolean:
            printf( "%d = %s\n", p.mVal, p.mVal ? "true" : "false" );
            break;

        case Type::Integer:
            switch (size)
            {
                case 0:
                case 1:
                    printf( "0x%0*x = %d\n", size * 2, p.mVal, (i8) p.mVal );
                    break;
                case 2:
                    printf( "0x%0*x = %d\n", size * 2, p.mVal, (i16) p.mVal );
                    break;
                case 3:
                case 4:
                    printf( "0x%0*x = %d\n", size * 2, *intU32ptr( p.mVal ),
                            *intI32ptr( p.mVal ) );
                    break;
                default:
                    printf( "0x%0*lx = %ld\n", size * 2, *intU64ptr( p.mVal ),
                            *intI64ptr( p.mVal ) );
                    break;
            }
            break;

        case Type::Unsigned:
            switch (size)
            {
                case 0:
                case 1:
                    printf( "0x%0*x = %u\n", size * 2, p.mVal, p.mVal );
                    break;
                case 2:
                    printf( "0x%0*x = %u\n", size * 2, p.mVal, p.mVal );
                    break;
                case 3:
                case 4:
                    printf( "0x%0*x = %u\n", size * 2, *intU32ptr( p.mVal ),
                            *intU32ptr( p.mVal ) );
                    break;
                default:
                    printf( "0x%0*lx = %lu\n", size * 2, *intU64ptr( p.mVal ),
                            *intU64ptr( p.mVal ) );
                    break;
            }
            break;

        default:
            puts( "?" );
            break;
    }
}
