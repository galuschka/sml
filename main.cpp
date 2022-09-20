#include "sml.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main( int argc, char ** argv )
{
    int fd = 0;
    if (argv[1])
        if ((fd = open( argv[1], O_RDONLY, 0 )) < 0) {
            printf( "can't open \"%s\"\n", argv[1] );
            exit (1);
        }

    Sml & sml = Sml::Instance();

    u8 byte;
    while (read( fd, &byte, 1 ) == 1) {
        sml.parse( byte );
    }
    sml.dump( "remaining data" );
    return (0);
}
