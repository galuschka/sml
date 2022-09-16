#include "sml.h"
#include "obis.h"
#include "filter.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

class SmlMqtt : public SmlFilter
{
        void filter( const char * serverId, Obj & objValList );
};

void SmlMqtt::filter( const char * serverId, Obj & objValList )
{
    printf( "%s = [\n", serverId );
    u8 cnt = 0;
    u8 len;
    for (u8 validx = 0; validx < objValList.size(); ++validx) {
        Obj objElem { objValList, validx };
        Obj objObjName { objElem, Obis::ListEntry::ObjName };
        const u8 *objName = objObjName.bytes( len );
        if (!objName || (len < 6))
            continue;

        if (objName[0] != 1)
            continue;
        if (objName[3] == 8) {
            // 1-*:{1,2}.8.*   Strom {Bezug,Einspeisung}
            if ((objName[2] < 1) || (objName[2] > 2))
                continue;
        } else if (objName[3] == 7) {
            // 1-*:{36,56,76}.7.*   Leistung 3 Phasen)
            u8 phase = (objName[2]-36)/20;
            if ((phase >= 3) || (objName[2] != ((phase*20)+36)))
                continue;
        } else {
            continue;
        }

        if (cnt++)
            printf(",\n");
        printf( " { name:  \"%d.%d.%d\"",
                    objName[2], objName[3], objName[4] );

        char buf[20];
        {
            Obj objValue { objElem, Obis::ListEntry::Value };
            printf( ",\n   value:  %s",
                    Obis::otoa( buf, sizeof(buf), objValue, true ) );
        }
        {
            Obj objScaler { objElem, Obis::ListEntry::Scaler };
            if (objScaler.typesize())
                printf( ",\n   scaler: %s",
                        Obis::otoa( buf, sizeof(buf), objScaler ) );
        }
        {
            Obj objUnit { objElem, Obis::ListEntry::Unit };
            if (objUnit.typesize())
                printf( ",\n   unit:   %s",
                        Obis::otoa( buf, sizeof(buf), objUnit ) );
        }
        {
            Obj objTime { objElem, Obis::ListEntry::ValTime };
            if (objTime.typesize())
                printf( ",\n   time:  \"%s\"",
                        Obis::otoa( buf, sizeof(buf), objTime ) );
        }
        printf( " }" );
    }
    printf( "\n]\n" );
}

int main( int argc, char ** argv )
{
    SmlMqtt sml { };

    u8 byte;
    while (read( 0, &byte, 1 ) == 1) {
        sml.parse( byte );
    }
    const u32 * errCnt = sml.getErrCntArray();
    printf( "valid packets: %4u\n",   errCnt[Err::NoError] );
    printf( "out of memory: %4u%s\n", errCnt[Err::OutOfMemory],
                                      errCnt[Err::OutOfMemory]
                                   ? " -> increase cMaxNofObj in sml.h!" : "" );
    printf( "type errors:   %4u\n",   errCnt[Err::InvalidType] );
    printf( "CRC errors:    %4u\n",   errCnt[Err::CrcError] );
    printf( "unknown errors:%4u\n",   errCnt[Err::Unknown] );

    return (0);
}
