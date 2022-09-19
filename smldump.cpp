#include "sml.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class SmlDump : public Sml
{
        void onReady( u8 err, u8 byte )
        {
            switch (err)
            {
                case Err::NoError:
                    break;

                case Err::OutOfMemory:
                    printf( "\n!!! out of memory: (offset %d -> increase cMaxNofObj = %d) !!!"
                            " (the following packet is invalid)\n",
                            mOffset, cMaxNofObj );
                    break;

                case Err::InvalidType:
                    printf( "\n!!! invalid type: char %02x at offset %d !!!"
                            " (the following packet is invalid)\n",
                            byte, mOffset );
                    break;

                case Err::CrcError:
                    printf( "\n!!! CRC error: read %04x / calc %04x !!!"
                            " (the following packet is invalid)\n",
                            mCrcRead, mCrc.get() );
                    break;

                default:
                    printf( "\n!!! unknown error %d: (char %02x at offset %d) !!!"
                            " (the following packet is invalid)\n",
                            err, byte, mOffset );
                    break;
            }
            dump();
            /*
             if (err == Err::NoError)
             exit( 0 );  // stop after 1st good frame
             */
        }
};

int main( int argc, char ** argv )
{
    int fd = 0;
    if (argv[1])
        if ((fd = open( argv[1], O_RDONLY, 0 )) < 0) {
            printf( "can't open \"%s\"\n", argv[1] );
            exit (1);
        }

    SmlDump sml { };

    u8 byte;
    while (read( fd, &byte, 1 ) == 1) {
        sml.parse( byte );
    }
    sml.dump( "remaining data" );
    return (0);
}
