#include "filter.h"
#include "obis.h"

#include <stdio.h> // snprintf()

void SmlFilter::onReady( u8 err, u8 byte )
{
    if (err != Err::NoError)
        return;

    char serverIdBuf[20];
    char *serverId = nullptr;

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
                // skip type char (i=id s=string x=hexdump)
                serverId = Obis::otoa( serverIdBuf, sizeof(serverIdBuf), objServerId, true ) + 1;
            }
                break;

            case Obis::MsgBody::MsgId::CloseResponse:
                break;

            case Obis::MsgBody::MsgId::GetListResponse:
                if (!serverId)
                    break;
                {
                    Obj objListRes { objBody, Obis::MsgBody::Message };
                    Obj objValList { objListRes, Obis::GetListRes::ValList };
                    filter( serverId, objValList );
                }
                break;

            default:
                break;
        }
    }
}
