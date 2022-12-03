#include "sml.h"
#include "obis.h"
#include <memory.h>

// DZG error: https://wiki.volkszaehler.org/hardware/channels/meters/power/edl-ehz/dzg_dvs74
//
// Value of 1-0:36.7.0*255": Sum active instantaneous power without reverse blockade (A+ - A-) in phase L1
// is exported by the smart meter as int32, while it is uint32

void Sml::fixup()
{
    static u8 measIdToFix[] = {1,0,36,7,0};

    if (mErr)
        return; // no fixup on any error

    u32 msgId = 0;
    for (u8 lvl0idx = 0; msgId != Obis::MsgBody::MsgId::CloseResponse; ++lvl0idx)
    {
        Obj objMsg { *this, lvl0idx };
        Obj objBody { objMsg, Obis::Msg::MessageBody };
        Obj objMsgId { objBody, Obis::MsgBody::MessageID };
        bool typematch;
        u32 msgId = objMsgId.getU32( typematch );
        if (! typematch)
            break;  // will break also, when objMsg, objBody or objMsgId is invalid
        if (msgId != Obis::MsgBody::MsgId::GetListResponse)
            continue;

        Obj objListRes { objBody, Obis::MsgBody::Message };
        Obj objValList { objListRes, Obis::GetListRes::ValList };

        for (u8 validx = 0; validx < objValList.size(); ++validx)
        {
            Obj objElem { objValList, validx };
            Obj objName { objElem, Obis::ListEntry::ObjName };
            u8 len;
            const u8 *measId = objName.bytes( len );
            if (len < 6)
                continue;

            // 1-0:36.7.0*255": Sum active instantaneous power without reverse blockade (A+ - A-) in phase L1
            static u8 measIdToFix[] = {1,0,36,7,0};
            if (memcmp( measId, measIdToFix, sizeof(measIdToFix)))
                continue;

            Obj objValue { objElem, Obis::ListEntry::Value };
            if (objValue.typesize() == Obj::typesize( Type::Integer, 2 ))
                mObjDef[ objValue.objIdx() ].mTypeSize = Obj::typesize( Type::Unsigned, 2 );
            return;
        }
    }
}
