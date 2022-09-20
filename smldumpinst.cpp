#include "smldump.h"

#include <stdio.h>

class SmlDumpInst : public SmlDump
{
        void onReady( u8 err, u8 byte );

    public:
        static SmlDumpInst & Instance();
};

SmlDumpInst & SmlDumpInst::Instance()
{
    static SmlDumpInst sml{};
    return sml;
}

Sml & Sml::Instance()
{
    return SmlDumpInst::Instance();
}

void SmlDumpInst::onReady( u8 err, u8 byte )
{
    switch (err)
    {
    case Err::NoError:
        break;

    case Err::OutOfMemory:
        printf("\n!!! out of memory: (offset %d -> increase cMaxNofObj = %d) !!!"
               " (the following packet is invalid)\n",
               mOffset, cMaxNofObj);
        break;

    case Err::InvalidType:
        printf("\n!!! invalid type: char %02x at offset %d !!!"
               " (the following packet is invalid)\n",
               byte, mOffset);
        break;

    case Err::CrcError:
        printf("\n!!! CRC error: read %04x / calc %04x !!!"
               " (the following packet is invalid)\n",
               mCrcRead, mCrc.get());
        break;

    default:
        printf("\n!!! unknown error %d: (char %02x at offset %d) !!!"
               " (the following packet is invalid)\n",
               err, byte, mOffset);
        break;
    }
    dump();
    /*
     if (err == Err::NoError)
     exit( 0 );  // stop after 1st good frame
     */
}
