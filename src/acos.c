/* acos (C89 7.5.2.2). For |x| <= 0.5, pi/2 - asin x; else
 * 2 atan(sqrt((1-|x|)/(1+|x|))), and pi minus that for x < 0. */

#include <errno.h>
#include "k.h"
#include "kc.h"

double __cdecl acos(double x)
{
    kdbl u;
    double a, r, h, e, p, t[2];
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
    if (a == 1.0) {
        if (!neg)
            return 0.0;
        return KC_PIH;
    }
    if (a <= 0.5) {
        kasq(a, 0.0, t);
        r = katan2(t[0], t[1]);         /* asin a */
        if (neg)
            r = -r;
        h = kd_sum(KC_PIO2H, -r, &e);
        return h + (e + KC_PIO2L);
    }
    /* v = (1 - a) / (1 + a) as a double-double (t[0], t[1]), then sqrt v */
    p = kd_sum(1.0, a, &e);             /* 1 + a = p + e */
    t[0] = (1.0 - a) / p;
    h = kd_prod(t[0], p, &r);
    t[1] = (((1.0 - a) - h) - r - t[0] * e) / p;
    h = ksqrt(t[0]);                    /* sqrt(v) = h + e */
    p = kd_prod(h, h, &r);
    e = (((t[0] - p) - r) + t[1]) / (2.0 * h);
    r = 2.0 * katan2(h, e);
    if (!neg)
        return r;
    h = kd_sum(KC_PIH, -r, &e);
    return h + (e + KC_PIL);
}
