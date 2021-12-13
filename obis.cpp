#include "sml.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

class SmlObis : public Sml
{
        enum MsgId
        {
            OpenRequest = 0x00000100,
            OpenResponse = 0x00000101,
            CloseRequest = 0x00000200,
            CloseResponse = 0x00000201,
            GetProfilePackRequest = 0x00000300,
            GetProfilePackResponse = 0x00000301,
            GetProfileListRequest = 0x00000400,
            GetProfileListResponse = 0x00000401,
            GetProcParameterRequest = 0x00000500,
            GetProcParameterResponse = 0x00000501,
            SetProcParameterRequest = 0x00000600,
            SetProcParameterResponse = 0x00000601,
            GetListRequest = 0x00000700,
            GetListResponse = 0x00000701,
            GetCosemRequest = 0x00000800,
            GetCosemResponse = 0x00000801,
            SetCosemRequest = 0x00000900,
            SetCosemResponse = 0x00000901,
            ActionCosemRequest = 0x00000A00,
            ActionCosemResponse = 0x00000A01,
            AttentionResponse = 0x0000FF01,
        };
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

        bool isascii( u8 byte )
        {
            enum
            {
                Space = 0x20,
                Del = 0x7f
            };
            return ((byte >= Space) && (byte <= Del));
        }

        bool isDeviceId( const Obj & obj )
        {
            if (!obj.isType( Type::ByteStr ))
                return false;
            u8 len;
            const u8 *s = obj.bytes( len );
            if (len < 5)
                return (false);
            if ((*s > 20) || (*s < 6))
                return (false);
            if (!(isascii( s[2] ) && isascii( s[3] ) && isascii( s[4] )))
                return (false);
            return (true);
        }

        void printAsDeviceId( const Obj & obj )
        {
            u8 len;
            const u8 *id = obj.bytes( len );
            u32 sno = 0;
            for (u8 i = 5; i < len; ++i) {
                sno <<= 8;
                sno |= id[i];
            }
            printf( "%d%.3s%0*u", id[1], id + 2, id[0], sno );
        }

        void printOptional( const char * prefix, const Obj & obj )
        {
            if (obj.typesize() != Type::ByteStr)
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
            if (isDeviceId( obj )) {
                printAsDeviceId( obj );
            } else {
                const u8 *b = obj.bytes( len );
                bool ascii = true;
                for (u8 i = 0; i < len; ++i) {
                    if (!isascii( b[i] )) {
                        ascii = false;
                        break;
                    }
                }
                if (ascii) {
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
    u32 msgId = 0;

    fputs( "{ ", stdout );

    for (u8 lvl0idx = 0; msgId != CloseResponse; ++lvl0idx) {
        /*
         * SML message ::= SEQUENCE
         * {
         *     0 transactionId       Octet String,
         *     1 groupNo             Unsigned8,
         *     2 abortOnError        Unsigned8,
         *  -> 3 messageBody         SML_MessageBody,
         *     4 crc16               Unsigned16,
         *     5 endOfSmlMsg         EndOfSmlMsg
         * }
         * message body ::= SEQUENCE
         * {
         *  -> 0 message ID
         *  -> 1 message
         * }
         */
        bool typematch;
        u8 len;
        Obj objMsg { *this, lvl0idx };
        Obj objBody { objMsg, 3 };    // body

        {
            Obj objMsgId { objBody, 0 };  //
            msgId = objMsgId.getU32( typematch );
            if (!typematch) {
                msgId = objMsgId.getU16( typematch );
                if (!typematch)
                    break;
            }
        }

        switch (msgId)
        {
            case OpenResponse: {
                /*
                 * SML_PublicOpen.Res ::= SEQUENCE
                 * {
                 *     0 codepage      Octet String OPTIONAL,
                 *     1 clientId      Octet String OPTIONAL,
                 *     2 reqFileId     Octet String,
                 *  -> 3 serverId      Octet String,
                 *     4 refTime       SML_Time OPTIONAL,
                 *     5 smlVersion    Unsigned8 OPTIONAL,
                 * }
                 */
                Obj objOpenres { objBody, 1 };
                Obj objDeviceID { objOpenres, 3 };
                printObj( "\"ID\"     : ", objDeviceID );
            }
                break;

            case CloseResponse:
                break;

            case GetListResponse:  // ListResponse
            {
                /*
                 * SML_GetList.Res ::= SEQUENCE
                 * {
                 *    0 clientId        Octet String OPTIONAL,
                 *    1 serverId        Octet String,
                 *    2 listName        Octet String OPTIONAL,
                 *    3 actSensorTime   SML_Time OPTIONAL,
                 * -> 4 valList         SML_List,
                 *    5 listSignature   SML_Signature OPTIONAL,
                 *    6 actGatewayTime  SML_Time OPTIONAL
                 * }
                 */
                /*
                 * SML_ListEntry ::= SEQUENCE
                 * {
                 *    0 objName         Octet String,
                 *    1 status          SML_Status OPTIONAL,
                 *    2 valTime         SML_Time OPTIONAL,
                 *    3 unit            SML_Unit OPTIONAL,
                 *    4 scaler          Integer8 OPTIONAL,
                 *    5 value           SML_Value,
                 *    6 valueSignature  SML_Signature OPTIONAL
                 * }
                 * objName o6
                 *    0- Medium      Elektrizität = 1, Gas = 7, Wasser, Wärme...
                 *    1: Kanal       interne oder externe Kanäle, nur bei mehreren Kanälen
                 *    2. Messgröße   Wirk-, Blind-, Scheinleistung, Strom, Spannung,...
                 *    3. Messart     Maximum, aktueller Wert, Energie...
                 *    4* Tarifstufe  Tarifstufe, z.B. Total, Tarif 1, Tarif 2...
                 *    5  Vorwertzählerstand 1- oder 2-stellig 00...99
                 */
                Obj objListRes { objBody, 1 };
                Obj objValList { objListRes, 4 };
                const char *sep = ",\n  \"values\" : [\n";
                const char *close = "";

                for (u8 validx = 0; validx < objValList.size(); ++validx) {
                    Obj objElem { objValList, validx };
                    {
                        Obj objName { objElem, 0 };
                        const u8 *measId = objName.bytes( len );
                        if (len < 6)
                            continue;

                        printf( "%s  { \"idnum\" : \"%d-%d:%d.%d.%d*%d\"", sep,
                                measId[0], measId[1], measId[2], measId[3],
                                measId[4], measId[5] );
                        sep = ",\n";
                        close = " ]";

                        bool known = false;
                        for (u8 eIdx = 0;
                                eIdx < (sizeof(IdTab) / sizeof(IdTab[0]));
                                ++eIdx) {
                            MeasId const &eRef = IdTab[eIdx];
                            bool match = true;
                            bool exact = true;
                            for (u8 j = 0; j < 3; ++j)
                                if (eRef.id[j] == 0xff)
                                    exact = false;
                                else if (measId[2 + j] != eRef.id[j])
                                    match = false;
                            if (match
                                    && (exact || known || (eRef.id[0] != 0xff))) {
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
                        Obj objUnit { objElem, 3 };
                        Obj objScaler { objElem, 4 };
                        Obj objValue { objElem, 5 };

                        known = false;

                        u8 unit = objUnit.getU8( typematch );
                        if (((measId[2] == 1) || (measId[2] == 2))
                                && (measId[3] == 8) && typematch
                                && (unit == 30)) {
                            u32 value = objValue.getU32( typematch );
                            if (!typematch)
                                value = objValue.getU16( typematch );
                            if (!typematch)
                                value = objValue.getU8( typematch );
                            if (typematch) {
                                i8 scale = 3 - objScaler.getI8( typematch );
                                if (typematch && (scale >= 0)) {
                                    known = true;
                                    fputs( ",\n    \"value\" : ", stdout );
                                    if (scale) {
                                        double x = value;
                                        for (u8 s = 0; s < scale; ++s)
                                            x /= 10.0;
                                        printf( "%.*f", scale, x );
                                    } else
                                        printf( "%d", value );
                                    fputs( ",\n    \"unit\"  : \"kWh\"",
                                    stdout );
                                }
                            }
                        }

                        if (!known) {
                            printObj( ",\n    \"value\" : ", objValue );
                            printOptional( ",\n    \"scale\" : ", objScaler );
                            printOptional( ",\n    \"unit\"  : ", objUnit );
                        }
#else
                    }
                    {
                        Obj objValue { objElem, 5 };
                        printObj( ",\n    \"value\" : ", objValue );
                    }
                    {
                        Obj objScaler { objElem, 4 };
                        printOptional( ",\n    \"scale\" : ", objScaler );
                    }
                    {
                        Obj objUnit { objElem, 3 };
                        printOptional( ",\n    \"unit\"  : ", objUnit );
#endif
                    }
                    fputs( " }", stdout );
                }  // for validx
                puts( close );
            }
                break;

            default:
                break;
        }
    }
    puts( "}" );
}

int main( int argc, char ** argv )
{
    SmlObis sml { };

    u8 byte;
    while (read( 0, &byte, 1 ) == 1) {
        sml.parse( byte );
    }
    return 0;
}
