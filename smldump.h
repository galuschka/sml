#include "sml.h"

class SmlDump : public Sml
{
        virtual void onReady( u8 err, u8 byte ) = 0;

    protected:
        void dump( const char * name = nullptr );
        void objDump( idx o, u8 indent );
};
