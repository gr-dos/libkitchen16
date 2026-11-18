/* ceil (C89 7.5.6.1): on the bits, no helper. */

#define KINT_BUMP
#include "kint.h"

double __cdecl ceil(double x)
{
    kdbl u;
    word frac[4];
    int e;

    u.d = x;
    e = (int)K_BE(u) - 1023;
    if (e >= 52)
        return x;               /* integral, infinity or NaN */
    if (e < 0) {
        if (K_ISZERO(u))
            return x;
        if (K_SIGN(u)) {
            K_SETZERO(u, 1);    /* ceil(-0.5) is -0 */
            return u.d;
        }
        u.d = 1.0;
        return u.d;
    }
    if (ktrunc(&u, e, frac) && !K_SIGN(u))
        kbump(&u, e);
    return u.d;
}
