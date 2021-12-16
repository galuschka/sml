#include "sml.h"
#include "obis.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

class SmlObis : public Sml
{
        struct MeasId
        {
                u8 id[3];
                const char *en;
                const char *de;
        };

        void onReady( u8 err, u8 byte )
        {
            switch (err)
            {
                case Err::NoError:
                    obis();
                    break;

                case Err::OutOfMemory:
                    printf( "\n!!! out of memory: (offset %d -> increase cMaxNofObj = %d) !!!"
                            " (the following packet is invalid)\n",
                            mOffset, cMaxNofObj );
                    dump();
                    break;

                case Err::InvalidType:
                    printf( "\n!!! invalid type: char %02x at offset %d !!!"
                            " (the following packet is invalid)\n",
                            byte, mOffset );
                    dump();
                    break;

                case Err::CrcError:
                    printf( "\n!!! CRC error: read %04x / calc %04x !!!"
                            " (the following packet is invalid)\n",
                            mCrcRead, mCrc.get() );
                    dump();
                    break;

                default:
                    printf( "\n!!! unknown error %d: (char %02x at offset %d) !!!"
                            " (the following packet is invalid)\n",
                            err, byte, mOffset );
                    dump();
                    break;
            }
        }

        void printOptional( const char * prefix, const Obj & obj )
        {
            if (obj.typesize() != Type::ByteStr)  // 0-sized byte string -> skip
                printObj( prefix, obj );
        }

        void printObj( const char * prefix, const Obj & obj );

        void obis();

        static const MeasId IdTab[];
};

const SmlObis::MeasId SmlObis::IdTab[] = {
// @fmt:off
        { {  96,  50,   1 }, "Manufacturer ID", "Herstellerkennung" },
        { {  96,   1,   0 }, "Device ID",       "Geräteidentifikation" },
        { {   0,   2,   0 }, "Firmware version","Firmware-Version" },
        { {   1,   8, 255 }, "Work purchase",   "Arbeit Bezug" },
        { {   2,   8, 255 }, "Work feed-in",    "Arbeit Einspeisung" },
        { {  16,   7, 255 }, "Current power",   "Momentane Wirkleistung" },
        { { 255, 255,   0 }, "No tariff",       "tariflos" },
        { { 255, 255,   1 }, "Tariff 1",        "Tarif 1" },
        { { 255, 255,   2 }, "Tariff 2",        "Tarif 2" },
};
// @fmt:on

void SmlObis::printObj( const char * prefix, const Obj & obj )
{
    u8 len;
    bool typematch;

    fputs( prefix, stdout );
    switch (obj.type())
    {
        case Type::ByteStr:
            putchar( '"' );
            if (Obis::isDeviceId( obj )) {
                char devId[20];
                printf( "%s", Obis::otoDevId( devId, sizeof(devId), obj ) );
            } else {
                const u8 *b = obj.bytes( len );
                if (Obis::isString( b, len )) {
                    printf( "%.*s", len, b );
                } else {
                    for (u8 i = 0; i < len; ++i)
                        printf( "%02x", b[i] );
                }
            }
            putchar( '"' );
            break;

        case Type::Boolean:
            fputs( obj.getU8( typematch ) ? "true" : "false", stdout );
            break;

        case Type::Integer:
            switch (obj.size())
            {
                case 1:
                    printf( "%d", obj.getI8( typematch ) );
                    break;
                case 2:
                    printf( "%d", obj.getI16( typematch ) );
                    break;
                case 3:
                case 4:
                    printf( "%d", obj.getI32( typematch ) );
                    break;
                default:
                    printf( "%ld", obj.getI64( typematch ) );
                    break;
            }
            break;

        case Type::Unsigned:
            switch (obj.size())
            {
                case 1:
                    printf( "%u", obj.getU8( typematch ) );
                    break;
                case 2:
                    printf( "%u", obj.getU16( typematch ) );
                    break;
                case 3:
                case 4:
                    printf( "%u", obj.getU32( typematch ) );
                    break;
                default:
                    printf( "%lu", obj.getU64( typematch ) );
                    break;
            }
            break;

        default:
            printf( "\"?\"" );
            break;
    }
}

void SmlObis::obis()
{
    fputs( "{ ", stdout );

    const char *const firstprefix = ",\n  \"values\" : [\n";
    const char *const contprefix = ",\n";
    const char *const postfix = " ]";
    const char *prefix = firstprefix;

    u32 msgId = 0;
    for (u8 lvl0idx = 0; msgId != Obis::MsgBody::MsgId::CloseResponse; ++lvl0idx) {
        bool typematch;
        u8 len;
        Obj objMsg { *this, lvl0idx };
        Obj objBody { objMsg, Obis::Msg::MessageBody };

        {
            Obj objMsgId { objBody, Obis::MsgBody::MessageID };
            msgId = objMsgId.getU32( typematch );
            if (!typematch) {
                msgId = objMsgId.getU16( typematch );
                if (!typematch)
                    break;
            }
        }

        switch (msgId)
        {
            case Obis::MsgBody::MsgId::OpenResponse: {
                Obj objOpenres { objBody, Obis::MsgBody::Message };
                Obj objServerId { objOpenres, Obis::OpenRes::ServerId };
                printObj( "\"SrvrId\" : ", objServerId );
            }
                break;

            case Obis::MsgBody::MsgId::CloseResponse:
                break;

            case Obis::MsgBody::MsgId::GetListResponse: {
                Obj objListRes { objBody, Obis::MsgBody::Message };
                Obj objValList { objListRes, Obis::GetListRes::ValList };

                for (u8 validx = 0; validx < objValList.size(); ++validx) {
                    Obj objElem { objValList, validx };
                    {
                        Obj objName { objElem, Obis::ListEntry::ObjName };
                        const u8 *measId = objName.bytes( len );
                        if (len < 6)
                            continue;

                        printf( "%s  { \"idnum\" : \"%d-%d:%d.%d.%d*%d\"", prefix, measId[0],
                                measId[1], measId[2], measId[3], measId[4], measId[5] );

                        prefix = contprefix;

                        bool known = false;
                        for (u8 eIdx = 0; eIdx < (sizeof(IdTab) / sizeof(IdTab[0])); ++eIdx) {
                            MeasId const &eRef = IdTab[eIdx];
                            bool match = true;
                            bool exact = true;
                            for (u8 j = 0; j < 3; ++j)
                                if (eRef.id[j] == 0xff)
                                    exact = false;
                                else if (measId[2 + j] != eRef.id[j])
                                    match = false;
                            if (match && (exact || known || (eRef.id[0] != 0xff))) {
                                if (!known)
                                    fputs( ",\n    \"descr\" : \"", stdout );
                                else
                                    fputs( " - ", stdout );
                                known = true;
                                fputs( eRef.en, stdout );
                            }
                            if (match && exact)
                                break;
                        }
                        if (known)
                            putchar( '"' );
#if 1
                        Obj objUnit { objElem, Obis::ListEntry::Unit };
                        Obj objScaler { objElem, Obis::ListEntry::Scaler };
                        Obj objValue { objElem, Obis::ListEntry::Value };

                        known = false;
                        do {  // while(false) - avoid goto
                            if ((measId[3] != 8) || (((measId[2] != 1) && (measId[2] != 2))))
                                break;

                            u8 unit = objUnit.getU8( typematch );
                            if (!typematch || (unit != Obis::ListEntry::Unit::Wh))
                                break;

                            i8 precision = 3 - objScaler.getI8( typematch );  // precision of kWh
                            if (!typematch || (precision < 0))
                                break;

                            if (!precision) {
                                printObj( ",\n    \"value\" : ", objValue );
                                fputs( ",\n    \"unit\"  : \"kWh\"", stdout );
                                known = true;
                                break;
                            }

                            u32 value = objValue.getU32( typematch );
                            if (!typematch) {
                                value = objValue.getU16( typematch );
                                if (!typematch) {
                                    value = objValue.getU8( typematch );
                                    if (!typematch)
                                        break;
                                }
                            }

                            char buf[12];
                            char *cp = Obis::utoa( buf + 1, sizeof(buf) - 1, value );
                            if (*cp == '#')
                                break;

                            char *point = buf + sizeof(buf) - 2 - precision;
                            while (cp > point)
                                *--cp = '0';    //      00xyz
                            for (char *bp = --cp; bp < point; ++bp)
                                bp[0] = bp[1];  // move integer part digits one ahead
                            *point = '.';       // rstuv.wxyz

                            fputs( ",\n    \"value\" : ", stdout );
                            fputs( cp, stdout );
                            fputs( ",\n    \"unit\"  : \"kWh\"", stdout );
                            known = true;

                        } while (false);

                        if (!known) {
                            printObj( ",\n    \"value\" : ", objValue );
                            printOptional( ",\n    \"scale\" : ", objScaler );
                            printOptional( ",\n    \"unit\"  : ", objUnit );
                        }
#else
                    }
                    {
                        Obj objValue { objElem, Obis::ListEntry::Value };
                        printObj( ",\n    \"value\" : ", objValue );
                    }
                    {
                        Obj objScaler { objElem, Obis::ListEntry::Scaler };
                        printOptional( ",\n    \"scale\" : ", objScaler );
                    }
                    {
                        Obj objUnit { objElem, Obis::ListEntry::Unit };
                        printOptional( ",\n    \"unit\"  : ", objUnit );
#endif
                    }
                    {
                        Obj objTime { objElem, Obis::ListEntry::ValTime };
                        printOptional( ",\n    \"time\"  : ", objTime );
                    }
                    fputs( " }", stdout );
                }  // for validx
            }
                break;

            default:
                break;
        }
    }
    puts( prefix == contprefix ? postfix : "" );
    puts( "}" );
}

int main( int argc, char ** argv )
{
    SmlObis sml { };

    u8 byte;
    while (read( 0, &byte, 1 ) == 1) {
        sml.parse( byte );
    }
    return (0);
}
