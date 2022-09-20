#pragma once

#include "sml.h"

class SmlFilter : public Sml
{
        void onReady( u8 err, u8 byte );
        virtual void filter( const char * devId, Obj & objValList ) = 0;
};
