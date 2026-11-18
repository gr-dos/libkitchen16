/* fabs (C89 7.5.6.2): on the bits, no helper. */

#include "k.h"

double __cdecl fabs(double x)
{
    kdbl u;

    u.d = x;
    u.w[3] &= 0x7FFF;
    return u.d;
}
