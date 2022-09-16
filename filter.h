#pragma once

#include "sml.h"

class SmlFilter : public Sml
{
        void onReady( u8 err, u8 byte );
        virtual void filter( const char * devId, Obj & objValList ) = 0;

        u32 mErrCnt[Err::Unknown + 1] {0};  // unknown errors accumulated to mErrCnt[Err::Unknown]

    public:
        const u32 * getErrCntArray() const { return mErrCnt; }
        u32 getErrCnt( u8 idx ) const { return mErrCnt[ idx < Err::Unknown ? idx : Err::Unknown ]; }
};
