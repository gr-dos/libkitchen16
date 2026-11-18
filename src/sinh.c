/* sinh (C89 7.5.3.2): a series below 1, else (e^|x| - e^-|x|) / 2 from
 * e^|x| as a double-double. */

#include <errno.h>
#include "k.h"
#include "kc.h"

double __cdecl sinh(double x)
{
    static const double S[11] = KC_SINHS;
    kdbl u;
    double a, y[2], eh, el, inv, z, s, r;
    int n, neg, i;

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    if (K_ISINF(u) || K_ISZERO(u) || K_BE(u) < 1023 - 27)
        return x;
    neg = K_SIGN(u);
    u.w[3] &= 0x7FFF;
    a = u.d;
    if (a < 1.0) {
        z = a * a;
        s = S[10];
        for (i = 9; i >= 0; i--)
            s = s * z + S[i];
        r = a + (a * z) * s;
    } else if (a > 710.4758600739439) {
        errno = ERANGE;
        K_SETINF(u, neg);
        return u.d;
    } else {
        n = kexp(a, 0.0, y);
        if (a > 40.0) {
            r = kscale(y[0], n - 1);
            u.d = r;
            if (K_ISINF(u))
                errno = ERANGE;
        } else {
            eh = kscale(y[0], n);
            el = kscale(y[1], n);
            inv = 1.0 / eh;
            inv -= inv * (el * inv);
            s = kd_sum(eh, -inv, &z);
            r = 0.5 * (s + (z + el));
        }
    }
    return neg ? -r : r;
}
