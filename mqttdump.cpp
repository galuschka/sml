#include "sml.h"
#include "obis.h"
#include "filter.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class SmlMqtt : public SmlFilter
{
        void filter( const char * serverId, Obj & objValList );
        void dump( const char * header = nullptr ) {}

    public:
        static SmlMqtt & Instance();
};

SmlMqtt & SmlMqtt::Instance()
{
    static SmlMqtt sml{};
    return sml;
}

Sml & Sml::Instance()
{
    return SmlMqtt::Instance();
}

void SmlMqtt::filter( const char * serverId, Obj & objValList )
{
    u8 len;
    for (u8 validx = 0; validx < objValList.size(); ++validx) {
        Obj objElem { objValList, validx };
        Obj objObjName { objElem, Obis::ListEntry::ObjName };
        const u8 *objName = objObjName.bytes( len );
        if (!objName || (len < 6))
            continue;

        if (objName[0] != 1)
            continue;

        char name[24];
        snprintf( name, sizeof(name), "%d-%d:%d.%d.%d*%d", objName[0], objName[1],
                    objName[2], objName[3], objName[4], objName[5] );

        char buf[20];
        {
            Obj objValue { objElem, Obis::ListEntry::Value };
            printf( "%s/%s/value %s\n", serverId, name,
                    Obis::otoa( buf, sizeof(buf), objValue, true ) );
        }
        {
            Obj objScaler { objElem, Obis::ListEntry::Scaler };
            if (objScaler.typesize())
                printf( "%s/%s/scaler %s\n", serverId, name,
                        Obis::otoa( buf, sizeof(buf), objScaler ) );
        }
        {
            Obj objUnit { objElem, Obis::ListEntry::Unit };
            if (objUnit.typesize())
                printf( "%s/%s/unit %s\n", serverId, name,
                        Obis::otoa( buf, sizeof(buf), objUnit ) );
        }
        {
            Obj objTime { objElem, Obis::ListEntry::ValTime };
            if (objTime.typesize())
                printf( "%s/%s/time %s\n", serverId, name,
                        Obis::otoa( buf, sizeof(buf), objTime ) );
        }
    }
}
