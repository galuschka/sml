#include "obisid.h"

namespace {
constexpr int triple( uint8_t x, uint8_t y, uint8_t z )
{
    return ((x << 16) | (y << 8) | z);
}
}

void Obis::id2string( std::string & out, const uint8_t * id )
{
    const uint8_t a = id[0];
 // const uint8_t b = id[1];
    const uint8_t c = id[2];
    const uint8_t d = id[3];
    const uint8_t e = id[4];

    const char * s1 = nullptr;

    out.clear();

    switch (triple( c, d, e ))
    {
        case triple(  0,  2, 0 ): s1 = "Firmware version"; break;
        case triple(  0,  2, 2 ): s1 = "Tariff program ID"; break;
        case triple( 96,  1, 0 ): s1 = "Device ID"; break;
        case triple( 96, 50, 1 ): s1 = "Manufacturer ID"; break;
    }
    if (s1) {
        out = s1;
        return;
    }
    if ((a != 1) || (c == 96)) {
        return;  // other than electricity or unknown 96.x.x
    }

    static const char * total   = " total";
    static const char * tariff  = " in tariff T";
    static const char * phase   = " in phase L";
    static const char * neutral = " in neutral";

    const char * s4 = nullptr;
    uint8_t      i4 = 0;

    if (e == 0) {
        if ((c < 20) && (d == 8)) {
            s4 = total;  // total over all tariffs
        } else {
            i4 = c / 20;  // 0..:total, 20..:L1, 40..:L1, 60..:L3, 80..:neutral
            if (i4) {
                s4 = phase;
                if (i4 == 4) {
                    s4 = neutral;
                    i4 = 0;
                }
            }
        }
    } else if (e < 10) {
        s4 = tariff;
        i4 = e;
    }
    const char * s2 = nullptr;
    const char * s3 = nullptr;
    switch (c % 20)
    {
        case  1: s1 = "Positive active";   s3 = " (A+)";  break;
        case  2: s1 = "Negative active";   s3 = " (A-)";  break;
        case  3: s1 = "Positive reactive"; s3 = " (Q+)";  break;
        case  4: s1 = "Negative reactive"; s3 = " (Q-)";  break;
        case  5: s1 = "Reactive";          s3 = " in Q1"; break;
        case  6: s1 = "Reactive";          s3 = " in Q2"; break;
        case  7: s1 = "Reactive";          s3 = " in Q3"; break;
        case  8: s1 = "Reactive";          s3 = " in Q4"; break;
        case  9: s1 = "Apparent";          s3 = " (S+)";  break;
        case 15: s1 = "Absolute active";   s3 = " (|A|)"; break;
        case 16: s1 = "Sum active";
                 s3 = " without reverse blockade (A+ - A-)"; break;
    }
    switch (d)
    {
        case 2: s2 = " cumulative maximum demand"; break;
        case 4: s2 = " demand in a current demand period"; break;
        case 5: s2 = " demand in the last completed demand period"; break;
        case 6: s2 = " maximum demand"; break;
        case 7: s2 = " instantaneous power"; break;
        case 8: s2 = " energy"; break;
        default: s2 = " ???"; break;
    }
    // some exceoptions from scheme:
    switch (d)
    {
        case 6:
            switch (c % 20)
            {
                case 11: s1 = "Maximum"; s2 = " current"; s3 = " (I max)"; break;
            }
            break;

        case 7:
            if ((s4 == phase) && (i4 == 0))
                s4 = nullptr;  // suppress "total" for cumulated values over all phases

            if (c == 81) {
                s1 = "Phase angle";
                s2 = nullptr;
                s3 = nullptr;
                s4 = nullptr;
                switch (e)
                {
                    case  1: s2 = " L1 to L2"; i4 = 2; break;
                    case  2: s2 = " L1 to L3"; i4 = 3; break;
                    case  4:
                    case 15:
                    case 26: s2 = " current to voltage";
                             s4 = phase;
                             i4 = (e / 11) + 1;
                             break;
                }
                break;
            }
            switch (c % 20)
            {
                case 11: s1 = "Instantaneous"; s2 = " current"; s3 = " (I)"; break;
                case 12: s1 = "Instantaneous"; s2 = " voltage"; s3 = " (U)"; break;
                case 13: s1 = "Instantaneous"; s2 = " power factor"; s3 = nullptr; break;
                case 14: s1 = "Frequency"; s2 = nullptr; s3 = nullptr; s4 = nullptr; break;
            }
            break;

        case 8:
            switch (c % 20)
            {
                case  5: s1 = "Imported inductive reactive";  s3 = nullptr; break;
                case  6: s1 = "Imported capacitive reactive"; s3 = nullptr; break;
                case  7: s1 = "Exported inductive reactive";  s3 = nullptr; break;
                case  8: s1 = "Exported capacitive reactive"; s3 = nullptr; break;
            }
            break;
    }
    if (! s1)
        return;

    out = s1;

    if (s2)
        out += s2;

    if (s3)
        out += s3;

    if (s4) {
        out += s4;
        if (i4)
            out += std::to_string( (int) i4 );
    }
}
