#include "obisunit.h"

namespace {
const char * s_unit[] = {
// code   unit         Unit name              Quantity               SI definition        (comment)
//=================================================================================================
  "\x01" "a"        // year                 t time                   52*7*24*60*60 s
 ,"\x02" "mo"       // month                t time                   31*24*60*60 s
 ,"\x03" "wk"       // week                 t time                   7*24*60*60 s
 ,"\x04" "d"        // day                  t time                   24*60*60 s
 ,"\x05" "h"        // hour                 t time                   60*60 s
 ,"\x06" "min."     // min                  t time                   60 s
 ,"\x07" "s"        // second               t time                   s
 ,"\x08" "°"        // degree                 (phase) angle          rad*180/π
 ,"\x09" "°C"       // degree celsius       T temperature            K-273.15
 ,"\x0a" "currency" //                        (local) currency
 ,"\x0b" "m"        // metre                l length                 m
 ,"\x0c" "m/s"      // metre per second     v speed                  m/s
 ,"\x0d" "m³"       // cubic metre          V volume                 m³
 ,"\x0e" "m³"       // cubic metre            corrected volume       m³
 ,"\x0f" "m³/h"     // cubic metre per hour   volume flux            m³/(60*60s)
 ,"\x10" "m³/h"     // cubic metre per hour   corrected volume flux  m³/(60*60s)
 ,"\x11" "m³/d"     //                        volume flux            m³/(24*60*60s)
 ,"\x12" "m³/d"     //                        corrected volume flux  m³/(24*60*60s)
 ,"\x13" "l"        // litre                  volume                 10-3 m³
 ,"\x14" "kg"       // kilogram             m mass
 ,"\x15" "N"        // newton               F force
 ,"\x16" "Nm"       // newtonmeter            energy                 J = Nm = Ws
 ,"\x17" "Pa"       // pascal               p pressure               N/m²
 ,"\x18" "bar"      // bar                  p pressure               10⁵ N/m²
 ,"\x19" "J"        // joule                  energy                 J = Nm = Ws
 ,"\x1a" "J/h"      // joule per hour         thermal power          J/(60*60s)
 ,"\x1b" "W"        // watt                 P active power           W = J/s
 ,"\x1c" "VA"       // volt-ampere          S apparent power
 ,"\x1d" "var"      // var                  Q reactive power
 ,"\x1e" "Wh"       // watt-hour          (A) active energy          W*(60*60s)
 ,"\x1f" "VAh"      // volt-ampere-hour       apparent energy        VA*(60*60s)
 ,"\x20" "varh"     // var-hour               reactive energy        var*(60*60s)
 ,"\x21" "A"        // ampere               I current                A
 ,"\x22" "C"        // coulomb              Q electrical charge      C = As
 ,"\x23" "V"        // volt                 U voltage                V
 ,"\x24" "V/m"      // volt per metre       E electr.field strength
 ,"\x25" "F"        // farad                C capacitance            C/V = As/V
 ,"\x26" "Ω"        // ohm                  R resistance             Ω = V/A
 ,"\x27" "Ωm²/m"    // Ωmresistivity        ρ
 ,"\x28" "Wb"       // weber                Φ magnetic flux          Wb = Vs
 ,"\x29" "T"        // tesla                B magnetic flux density  Wb/m2
 ,"\x2a" "A/m"      // ampere per metre     H magn. field strength   A/m
 ,"\x2b" "H"        // henry                L inductance             H = Wb/A
 ,"\x2c" "Hz"       // hertz              f,ω frequency              1/s
 ,"\x2d" "1/(Wh)"   //                        R_W                           (active energy meter constant or pulse value)
 ,"\x2e" "1/(varh)" //                        R_B                           (reactive energy meter constant or pulse value)
 ,"\x2f" "1/(VAh)"  //                        R_S                           (apparent energy meter constant or pulse value)
 ,"\x30" "V²h"      // volt-squaredhours      volt-squared hour      V²(60*60s)
 ,"\x31" "A²h"      // ampere-squaredhours    ampere-squared hour    A²(60*60s)
 ,"\x32" "kg/s"     // kilogram per second    mass flux              kg/s
 ,"\x33" "S"        // siemens                conductance            1/Ω    (mho = reverse ohm)
 ,"\x34" "K"        // kelvin               T temperature
 ,"\x35" "1/(V²h)"  //                        R_U²h                         (Volt-squared hour meter constant or pulse value)
 ,"\x36" "1/(A²h)"  //                        R_I²h                         (Ampere-squared hour meter constant or pulse value)
 ,"\x37" "1/m³"     //                        R_V, pulse value              (volume meter constant or pulse value)
 ,"\x38" "%"        // %                      percentage
 ,"\x39" "Ah"       // ampere-hour            ampere-hours
 ,"\x3c" "Wh/m³"    //                        energy per volume      3,6*103 J/m³
 ,"\x3d" "J/m³"     //                        calorific value, wobbe
 ,"\x3e" "Mol %"    // mole percent           molar fraction of             (Basic gas composition unit) / gas composition
 ,"\x3f" "g/m³"     //                        mass density                  (Gas analysis, accompanying elements)
 ,"\x40" "Pa s"     // pascal second          dynamic viscosity             (Characteristic of gas stream)
 ,"\xfd" "(-)"      // reserved
 ,"\xfe" "?"        // other unit
 ,"\xff"            // unitless, count                                      (unit "")
};
}

const char * Obis::unit2string( uint8_t unit )
{
    int a = 0;
    int b = sizeof(s_unit) / sizeof(s_unit[0]) - 1;
    do {
        int const m = (a + b) / 2;
        int const c = unit - (uint8_t) s_unit[m][0];
        if (c == 0)
            return & s_unit[m][1];
        if (c < 0)
            b = m - 1;
        else
            a = m + 1;
    } while (a <= b);
    return nullptr;
}

#if 0  // check
#include <stdio.h>
int main( int argc, char **argv )
{
    uint8_t unit = 0;
    do {
        const char * str = Obis::unit2string( unit );
        if (str)
            printf( "%3d: %s\n", unit, str );
    } while (++unit);
    return 0;
}
#endif
