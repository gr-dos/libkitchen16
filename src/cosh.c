/* cosh (C89 7.5.3.1): (e^|x| + e^-|x|) / 2 from e^|x| as a double-double; a series below 1. */

#include <errno.h>
#include "k.h"
#include "kc.h"

double __cdecl cosh(double x)
{
    kdbl u;
    double a, y[2], eh, el, inv, r, z, s;
    static const double C[10] = KC_COSHC;
    int i;
    int n;

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    if (K_ISINF(u)) {
        K_SETINF(u, 0);
        return u.d;
    }
    u.w[3] &= 0x7FFF;
    a = u.d;
    if (K_BE(u) < 1023 - 27)
        return 1.0;
    if (a > 710.4758600739439) {
        errno = ERANGE;
        K_SETINF(u, 0);
        return u.d;
    }
    if (a < 1.0) {                      /* 1 + z/2 + z^2 C(z) */
        z = a * a;
        s = C[9];
        for (i = 8; i >= 0; i--)
            s = s * z + C[i];
        return 1.0 + (0.5 * z + (z * z) * s);
    }
    n = kexp(a, 0.0, y);
    if (a > 40.0) {
        r = kscale(y[0], n - 1);
        u.d = r;
        if (K_ISINF(u))
            errno = ERANGE;
        return r;
    }
    eh = kscale(y[0], n);
    el = kscale(y[1], n);
    inv = 1.0 / eh;
    inv -= inv * (el * inv);
    s = kd_sum(eh, inv, &z);
    return 0.5 * (s + (z + el));
}
