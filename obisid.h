#pragma once

#include <stdint.h>
#include <string>

// obisid2string: construct description by given OBIS ID
// measid contains the abcdef of the OBIS ID
// 
namespace Obis {
void id2string( std::string & out, const uint8_t * measid );
}
