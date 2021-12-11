
#include "sml.h"
#include <unistd.h>

int main( int argc, char ** argv )
{
    Sml sml{};

    u8 byte;
    while (read( 0, & byte, 1 ) == 1) {
        sml.parse( byte );
    }
    return 0;
}
