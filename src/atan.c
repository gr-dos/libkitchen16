/* atan (C89 7.5.2.3). */

#include "k.h"

double __cdecl atan(double x)
{
    kdbl u;
    double r;
    int neg;

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    if (K_ISZERO(u) || K_BE(u) < 1023 - 28)
        return x;                       /* atan x = x for |x| < 2^-28 */
    neg = K_SIGN(u);
    u.w[3] &= 0x7FFF;
    r = katan2(u.d, 0.0);
    return neg ? -r : r;
}
