#include "obis.h"
#include "sml.h"

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

}
