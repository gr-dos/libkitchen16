/* log10 (C89 7.5.4.5): ln x as a double-double times log10(e) as one, so
 * that log10 of the double nearest 10^n is exactly n. */

#include <errno.h>
#include "k.h"
#include "kc.h"

double __cdecl log10(double x)
{
    kdbl u;
    double r[2], p, e;

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    if (K_ISZERO(u)) {
        errno = ERANGE;
        K_SETINF(u, 1);
        return u.d;
    }
    if (K_SIGN(u)) {
        errno = EDOM;
        K_SETNAN(u);
        return u.d;
    }
    if (K_ISINF(u))
        return x;
    klog(x, r);
    p = kd_prodc(r[0], KC_L10EH, KC_L10EH_H, KC_L10EH_L, &e);
    e += r[0] * KC_L10EL + r[1] * KC_L10EH;
    return p + e;
}
