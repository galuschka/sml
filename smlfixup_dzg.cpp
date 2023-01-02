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

            if (memcmp( measId, measIdToFix, sizeof(measIdToFix)))
                continue;

            Obj objValue { objElem, Obis::ListEntry::Value };
            if (objValue.isType( Type::Integer ))
            {
                // here we toogle signed to unsigned encoding:
                mObjDef[ objValue.objIdx() ].mTypeSize ^= Obj::typesize( Type::Unsigned ^ Type::Integer, 0 );

                // When we got i8  (type 0x52) and bit  8 was set (e.g. c4),       we have set the i16 value to 0xffc4
                // When we got i24 (type 0x54) and bit 23 was set (e.g. 92 34 56), we have set the i32 value to 0xff923456
                // etc. for long long

                // Here we undo filling the leading 0xff's in these cases:
                switch (objValue.size())
                {
                    case 1:     mObjDef[ objValue.objIdx() ].mVal  &= 0xff; break;
                    case 3: *(intU32ptr( objValue.objDef().mVal )) &= 0xffffff; break;
                    case 5: *(intU64ptr( objValue.objDef().mVal )) &= 0xffffffffff; break;
                    case 6: *(intU64ptr( objValue.objDef().mVal )) &= 0xffffffffffff; break;
                    case 7: *(intU64ptr( objValue.objDef().mVal )) &= 0xffffffffffffff; break;
                }
            }
            return;
        }
    }
}
