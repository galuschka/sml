#include "obis.h"
#include "sml.h"

#include <string.h>

namespace Obis {

bool isAscii( u8 byte )
{
    enum
    {
        Space = 0x20,
        Del = 0x7f
    };
    return ((byte >= Space) && (byte < Del));
}

bool isString( const u8 * bytes, u8 len )
{
    while (len--)
        if (!isAscii( *bytes++ ))
            return (false);
    return (true);
}

constexpr char hexchar( u8 nibble )
{
    return (nibble + '0' + ((nibble / 10) * ('A' - '0')));
}

char* otoa( char * buf, u8 size, const Obj & obj, bool checkDevId )
{
    u8 len;
    bool typematch;

    switch (obj.type())
    {
        case Type::ByteStr: {
            if (checkDevId && isDeviceId( obj )) {
                char *devId = otoDevId( buf + 1, size - 1, obj );
                *--devId = 'i';
                return (devId);
            }

            const u8 *b = obj.bytes( len );
            if ((size >= (len + 2)) && isString( b, len )) {
                *buf = 's';
                memcpy( buf + 1, b, len );
                buf[len + 1] = 0;
                return (buf);
            }
            if (size < (len * 2 + 2)) {
                memset( buf, '#', size - 1 );
                buf[size - 1] = 0;
                return (buf);
            }
            *buf = 'x';
            for (u8 i = 0; i < len; ++i) {
                buf[i * 2 + 1] = hexchar( b[i] >> 4 );
                buf[i * 2 + 2] = hexchar( b[i] & 15 );
            }
            buf[len * 2 + 1] = 0;
            return (buf);
        }
            break;

        case Type::Boolean:
            strncpy( buf, obj.getU8( typematch ) ? "true" : "false", size - 1 );
            return buf;
            break;

        case Type::Integer:
            switch (obj.size())
            {
                case 1:
                    return (itoa( buf, size, obj.getI8( typematch ) ));
                case 2:
                    return (itoa( buf, size, obj.getI16( typematch ) ));
                case 3:
                case 4:
                    return (itoa( buf, size, obj.getI32( typematch ) ));
                default:
                    return (itoa( buf, size, obj.getI64( typematch ) ));
            }
            break;

        case Type::Unsigned:
            switch (obj.size())
            {
                case 1:
                    return (utoa( buf, size, obj.getU8( typematch ) ));
                case 2:
                    return (utoa( buf, size, obj.getU16( typematch ) ));
                case 3:
                case 4:
                    return (utoa( buf, size, obj.getU32( typematch ) ));
                default:
                    return (utoa( buf, size, obj.getU64( typematch ) ));
            }
            break;
        default:
            break;
    }
    buf[0] = 0;
    return (buf);
}

bool isDeviceId( const Obj & obj )
{
    if (!obj.isType( Type::ByteStr ))
        return (false);
    u8 len;
    const u8 *s = obj.bytes( len );
    if (len < 5)
        return (false);
    if ((*s > 20) || (*s < 6))
        return (false);
    if (!Obis::isString( s + 2, 3 ))
        return (false);
    return (true);
}

char* otoDevId( char * buf, u8 size, const Obj & obj )
{
    u8 len;
    const u8 *id = obj.bytes( len );
    if (size < (id[0] + 6)) {
        memset( buf, '#', size - 1 );
        buf[size - 1] = 0;
        return (buf);
    }
    u32 sno = 0;
    for (u8 i = 5; i < len; ++i) {
        sno <<= 8;
        sno |= id[i];
    }
    char *devId = utoa( buf, size, sno );
    if (*devId == '#')
        return devId;
    while (devId >= (buf + size - id[0]))
        *--devId = '0';
    *--devId = id[4];
    *--devId = id[3];
    *--devId = id[2];
    *--devId = (id[1] % 10) + '0';
    if (id[1] / 10) {
        *--devId = (id[1] / 10) + '0';
    }
    return devId;
}

// valScalToA - value and scaler to ascii:
// buf/size     output buffer and size
// objValue     object to value
// objScaler    object to scaler
// sep          nullptr or "" or ...
//              "."  ->  1234.56 (use decimal point and no thousands sep)
//              ",." -> 1,234.56 (use decimal point and thousands comma sep)
//              ","  ->  1234,56 (use german decimal comma and no thousands sep)
//              ".," -> 1.234,56 (use german decimal comma and thousands dot sep)
//
char * valScalToA( char * buf, u8 size, const Obj & objValue, const Obj & objScaler, const char *sep )
{
    char decSep = '.';
    char kiloSep = 0;
    if (sep && *sep) {
        if (sep[1]) {
            kiloSep = sep[0];
            decSep = sep[1];
        } else
            decSep = *sep;
    }

    bool typematch;
    i8 const scaler = objScaler.getI8( typematch );
    if (! typematch)
        return nullptr;  // no scaler or scaler type mismatch

    u32 const u = objValue.getU32( typematch );
    char *cp;
    if (typematch)
        cp = Obis::utoa( buf + 1, size - 1, u );
    else {
        i32 const i = objValue.getI32( typematch );
        if (typematch)
            cp = Obis::itoa( buf + 1, size - 1, i );
        else
            return nullptr;  // type mismatch
    }
    if (*cp == '#')
        return nullptr;  // overflow

    char * const end = buf + size - 1;
    char * point = end;
    if (scaler < 0) {
        point = end - 1 + scaler;
        while (cp > point)
            *--cp = '0';    // add some leading zeros (example 00xyz -> 0.0xyz)
        for (char *bp = --cp; bp < point; ++bp)
            bp[0] = bp[1];  // move integer part digits one ahead (?uvwxyz -> uvvwxyz)
        *point = decSep;    // uv.wxyz
    }
    else if (scaler > 0) {
        cp -= scaler;
        if (cp < buf)
            return nullptr;  // overflow
        char * zeros = end - scaler;
        for (char *bp = cp; bp < zeros; ++bp)
            *bp = bp[scaler];
        while (zeros < end)
            *zeros++ = 0;
    }
    if (kiloSep) {
        // how many kilo seps?
        u8 thousands = ((point - 1) - (cp + (*cp == '-' ? 1 : 0))) / 3;
        if (thousands) {
            cp -= thousands;
            if (cp < buf)
                return nullptr;  // overflow
            char *kp = point - (4 * thousands);
            for (char *bp = cp; bp < kp; ++bp)
                *bp = bp[thousands];
            for (u8 t = thousands; --t; kp += 4) {
                kp[1] = kp[1 + t];
                kp[2] = kp[2 + t];
                kp[3] = kp[3 + t];
            }
            for (kp = point - 4; thousands--; kp -= 4)
                *kp = kiloSep;
        }
    }
    return cp;
}

}
