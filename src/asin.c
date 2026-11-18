/* asin (C89 7.5.2.1). For |x| < 0.75, atan(x / sqrt(1 - x^2)); above,
 * pi/2 - 2 asin(sqrt((1-|x|)/2)), the argument of the inner one at most
 * 0.354. */

#include <errno.h>
#include "k.h"
#include "kc.h"

double __cdecl asin(double x)
{
    kdbl u;
    double a, s, e, r, t[2];
    int neg;

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    neg = K_SIGN(u);
    u.w[3] &= 0x7FFF;
    a = u.d;
    if (a > 1.0) {
        errno = EDOM;
        K_SETNAN(u);
        return u.d;
    }
    if (K_ISZERO(u) || K_BE(u) < 1023 - 28)
        return x;
    if (a < 0.75) {
        kasq(a, 0.0, t);
        r = katan2(t[0], t[1]);
    } else if (a == 1.0) {
        r = KC_PIO2H;
    } else {
        a = (1.0 - a) * 0.5;            /* exact */
        s = ksqrt(a);
        r = kd_prod(s, s, &e);
        e = ((a - r) - e) / (2.0 * s);  /* sqrt(a) = s + e */
        kasq(s, e, t);
        r = 2.0 * katan2(t[0], t[1]);
        s = kd_sum(KC_PIO2H, -r, &e);
        r = s + (e + KC_PIO2L);
    }
    return neg ? -r : r;
}
