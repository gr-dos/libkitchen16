/* tanh (C89 7.5.3.3): (e^2|x| - 1) / (e^2|x| + 1), with e^2|x| - 1 from the
 * double-double of kexp; +-1 from |x| >= 20. */

#include "k.h"

double __cdecl tanh(double x)
{
    kdbl u;
    double a, y[2], eh, el, dh, dl, q, p;
    int n, neg;

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    neg = K_SIGN(u);
    if (K_ISINF(u) || (u.w[3] & 0x7FFF) >= 0x4034) {    /* |x| >= 20 */
        q = neg ? -1.0 : 1.0;
        return q;
    }
    if (K_ISZERO(u) || K_BE(u) < 1023 - 27)
        return x;
    u.w[3] &= 0x7FFF;
    a = u.d;
    n = kexp(2.0 * a, 0.0, y);
    if (n == 0) {                       /* e^2a - 1 = eh + el, exactly */
        eh = y[0] - 1.0;
        el = y[1];
    } else {
        eh = kscale(y[0], n) - 1.0;
        el = kscale(y[1], n);
    }
    dh = kd_sum(eh, 2.0, &dl);          /* eh + 2 + el = dh + dl */
    dl += el;
    q = eh / dh;                        /* (eh + el) / (dh + dl) */
    p = kd_prod(q, dh, &a);
    q += (((eh - p) - a) + (el - q * dl)) / dh;
    return neg ? -q : q;
}
