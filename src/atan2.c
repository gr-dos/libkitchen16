/* atan2 (C89 7.5.2.4): the IEEE values for zeros and infinities, no error. */

#include "k.h"
#include "kc.h"

double __cdecl atan2(double y, double x)
{
    kdbl uy, ux;
    double ay, ax, r, h, e, q;
    int sy, sx;

    uy.d = y;
    ux.d = x;
    if (K_ISNAN(uy)) {
        K_QUIET(uy);
        return uy.d;
    }
    if (K_ISNAN(ux)) {
        K_QUIET(ux);
        return ux.d;
    }
    sy = K_SIGN(uy);
    sx = K_SIGN(ux);
    if (K_ISZERO(uy)) {
        if (!sx)
            return y;                   /* +-0 */
        r = KC_PIH;
        return sy ? -r : r;
    }
    if (K_ISZERO(ux)) {
        r = KC_PIO2H;
        return sy ? -r : r;
    }
    if (K_ISINF(ux)) {
        if (K_ISINF(uy))
            r = sx ? KC_3PIO4H : KC_PIO4H;
        else
            r = sx ? KC_PIH : 0.0;
        return sy ? -r : r;
    }
    if (K_ISINF(uy)) {
        r = KC_PIO2H;
        return sy ? -r : r;
    }
    uy.w[3] &= 0x7FFF;
    ux.w[3] &= 0x7FFF;
    ay = uy.d;
    ax = ux.d;
    q = ay / ax;                        /* with its remainder, for atan */
    if (ax < 1.0e290 && q > 1.0e-290) { /* else q is exact enough, atan q = q */
        h = kd_prod(q, ax, &e);
        e = ((ay - h) - e) / ax;
    } else {
        e = 0.0;
    }
    r = katan2(q, e);                   /* in [0, pi/2] */
    if (sx) {                           /* pi - r */
        h = kd_sum(KC_PIH, -r, &e);
        r = h + (e + KC_PIL);
    }
    return sy ? -r : r;
}
