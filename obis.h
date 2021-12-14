#pragma once

#include <type_traits>  // make_unsigned
#include "sml.h"

//@fmt:off

namespace Obis {

namespace Msg {
enum Msg
{
    TransactionID   = 0,    // Octet String
    GroupNo         = 1,    // Unsigned8
    AbortOnError    = 2,    // Unsigned8
    MessageBody     = 3, // <- SML_MessageBody
    Crc16           = 4,    // Unsigned16
    EndOfSmlMsg     = 5,    // EndOfSmlMsg
};
}

namespace MsgBody {
enum MsgBody
{
    MessageID   = 0,    // message ID
    Message     = 1,    // message
};
namespace MsgId {
enum MsgId
{
    OpenRequest              = 0x00000100,
    OpenResponse             = 0x00000101,
    CloseRequest             = 0x00000200,
    CloseResponse            = 0x00000201,
    GetProfilePackRequest    = 0x00000300,
    GetProfilePackResponse   = 0x00000301,
    GetProfileListRequest    = 0x00000400,
    GetProfileListResponse   = 0x00000401,
    GetProcParameterRequest  = 0x00000500,
    GetProcParameterResponse = 0x00000501,
    SetProcParameterRequest  = 0x00000600,
    SetProcParameterResponse = 0x00000601,
    GetListRequest           = 0x00000700,
    GetListResponse          = 0x00000701,
    GetCosemRequest          = 0x00000800,
    GetCosemResponse         = 0x00000801,
    SetCosemRequest          = 0x00000900,
    SetCosemResponse         = 0x00000901,
    ActionCosemRequest       = 0x00000A00,
    ActionCosemResponse      = 0x00000A01,
    AttentionResponse        = 0x0000FF01,
};
}
}

namespace OpenRes {
enum OpenRes {
    CodePage    = 0,    // Octet String OPTIONAL
    ClientId    = 1,    // Octet String OPTIONAL
    ReqFileId   = 2,    // Octet String
    ServerId    = 3, // <- Octet String
    RefTime     = 4,    // SML_Time OPTIONAL
    SmlVersion  = 5,    // Unsigned8 OPTIONAL
};
}

namespace GetListRes {
enum GetList {
    ClientId        = 0,    // Octet String OPTIONAL
    ServerId        = 1,    // Octet String
    ListName        = 2,    // Octet String OPTIONAL
    ActSensorTime   = 3,    // SML_Time OPTIONAL
    ValList         = 4, // <- SML_List
    ListSignature   = 5,    // SML_Signature OPTIONAL
    ActGatewayTime  = 6,    // SML_Time OPTIONAL
};
}
namespace ListEntry {
enum ListEntry {
    ObjName         = 0, // <- Octet String
    Status          = 1,    // SML_Status OPTIONAL
    ValTime         = 2, // <- SML_Time OPTIONAL
    Unit            = 3, // <- SML_Unit OPTIONAL
    Scaler          = 4, // <- Integer8 OPTIONAL
    Value           = 5, // <- SML_Value
    ValueSignature  = 6,    // SML_Signature OPTIONAL
};
enum Unit {
    Wh = 30,
};
}
/* objName o6
 *    0- Medium      Elektrizität = 1, Gas = 7, Wasser, Wärme...
 *    1: Kanal       interne oder externe Kanäle, nur bei mehreren Kanälen
 *    2. Messgröße   Wirk-, Blind-, Scheinleistung, Strom, Spannung,...
 *    3. Messart     Maximum, aktueller Wert, Energie...
 *    4* Tarifstufe  Tarifstufe, z.B. Total, Tarif 1, Tarif 2...
 *    5  Vorwertzählerstand 1- oder 2-stellig 00...99
 */

bool isAscii( u8 byte );
bool isString( const u8 * bytes, u8 len );

template<typename T> char * utoa( char * buf, u8 size, T x )
{
    char * cp = buf + size - 1;
    *cp = 0; // terminator
    do {
        if (cp == buf) { // overflow
            // memset( buf, '#', len - 1 );
            cp = buf + size - 2;
            while (cp > buf)
                *--cp = '#';
            return (buf);
        }
        *--cp = (x % 10) + '0';
        x /= 10;
    } while (x);
    return (cp);
}

template<typename T> char * itoa( char * buf, u8 size, T x )
{
    typedef typename std::make_unsigned<T>::type unsignedT;

    if (x >= 0)
        return (mkUString( buf, size, static_cast<unsignedT>(x) ));

    char * s = mkUString( buf + 1, size - 1, static_cast<unsignedT>(-x) );
    *--s = '-';
    return (s);
}

} // namespace Obis

//@fmt:on
