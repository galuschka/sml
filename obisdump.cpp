#include "sml.h"
#include "obis.h"
#include "obisid.h"
#include "obisunit.h"
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
};

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
        // objMsg.dump( "objMsg" );
        Obj objBody { objMsg, Obis::Msg::MessageBody };
        // objBody.dump( "objBody" );

        {
            Obj objMsgId { objBody, Obis::MsgBody::MessageID };
            // objMsgId.dump( "objMsgId" );

            msgId = objMsgId.getU32( typematch );
            // printf( "msgid u32: %u (%d)\n", msgId, typematch );
            if (!typematch) {
                msgId = objMsgId.getU16( typematch );
                // printf( "msgid u16: %u (%d)\n", msgId, typematch );
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

                        printf( "%s  { \"idnum\": \"%d-%d:%d.%d.%d*%d\"", prefix, measId[0],
                                measId[1], measId[2], measId[3], measId[4], measId[5] );

                        prefix = contprefix;

                        std::string descr;
                        Obis::id2string( descr, measId );
                        if (! descr.empty()) {
                            fputs( ", \"descr\": \"", stdout );
                            fputs( descr.c_str(), stdout );
                            putchar( '"' );
                        }
#if 1
                        Obj objUnit { objElem, Obis::ListEntry::Unit };
                        Obj objScaler { objElem, Obis::ListEntry::Scaler };
                        Obj objValue { objElem, Obis::ListEntry::Value };

                        i8 scaler = objScaler.getI8( typematch );
                        if (!typematch)
                            scaler = 0;

                        u8 unit = objUnit.getU8( typematch );
                        const char * unitStr = Obis::unit2string( unit );

                        if (unit == Obis::ListEntry::Unit::Wh) {
                            scaler -= 3;
                            unitStr = "kWh";
                        }

                        if (!scaler) {
                            printObj( ", \"value\": ", objValue );
                            if (typematch) {
                                fputs( ", \"unit\": ", stdout );
                                if (unitStr)
                                    printf( "\"%s\"", unitStr );
                                else
                                    printf( "%d", unit );
                            }
                        } else {
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

                            char *point = buf + sizeof(buf) - 2 + scaler;
                            while (cp > point)
                                *--cp = '0';    //      00xyz
                            for (char *bp = --cp; bp < point; ++bp)
                                bp[0] = bp[1];  // move integer part digits one ahead
                            *point = '.';       // rstuv.wxyz

                            fputs( ", \"value\": ", stdout );
                            fputs( cp, stdout );
                            if (typematch) {
                                fputs( ", \"unit\": ", stdout );
                                if (unitStr)
                                    printf( "\"%s\"", unitStr );
                                else
                                    printf( "%d", unit );
                            }
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
                        printOptional( ", \"time\": ", objTime );
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
