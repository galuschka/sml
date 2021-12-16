#pragma once

#include "sml.h"

class SmlFilter : public Sml
{
        void onReady( u8 err, u8 byte );
        virtual bool filter( const u8 * objName, u8 len ) = 0;
        virtual void onFilter( const char * devId, const char * name, Obj & objGetList ) = 0;
};
