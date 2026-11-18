/* kpowi: x^y for a finite x != 0 and an integer y, 1 <= |y| <= 1024, by
 * repeated squaring in double-double (about 100 bits) with the exponent
 * kept apart, so that the result is correctly rounded: exact whenever the
 * power is a double. */

#include "k.h"
#include "kdd.h"

/* (h, l) = (h1, l1) * (h2, l2), renormalised to [0.707, 1.414) by a power
 * of two that is added to *ex */
#define DDMUL(h1, l1, h2, l2, h, l, ex) { \
    pp = kd_prod(h1, h2, &pe); \
    pe += (h1) * (l2) + (l1) * (h2); \
    K_FASTSUM(pp, pe, h, l) \
    if ((h) > 1.4142135623730951) { (h) *= 0.5; (l) *= 0.5; (ex)++; } \
    else if ((h) < 0.7071067811865475) { (h) *= 2.0; (l) *= 2.0; (ex)--; } }

double kpowi(double x, int y)
{
    kdbl u;
    double m, t, est, bh, bl, rh, rl, pp, pe;
    int be, kx, eb, er, n, neg;

    u.d = x;
    neg = K_SIGN(u) && (y & 1);
    u.w[3] &= 0x7FFF;
    be = K_BE(u);
    kx = 0;
    if (be == 0) {                      /* subnormal */
        u.d *= 18014398509481984.0;     /* 2^54 */
        be = K_BE(u);
        kx = -54;
    }
    kx += be - 1023;
    u.w[3] = (u.w[3] & 0x800F) | 0x3FF0;
    m = u.d;                            /* [1, 2) */
    if (m > 1.4142135623730951) {
        u.w[3] = (u.w[3] & 0x800F) | 0x3FE0;
        m = u.d;
        kx++;
    }
    n = y < 0 ? -y : y;
    /* a crude log2 of the result: n (kx + log2 m) */
    t = m - 1.0;
    est = (double)n * ((double)kx + t * (1.4426950408889634 - t * (0.7213475204444817 - t * 0.4808983469629878)));
    if (est > 1150.0 || est < -1150.0) {
        if ((est > 0.0) == (y > 0))
            K_SETINF(u, 0);
        else
            K_SETZERO(u, 0);
        est = u.d;
        return neg ? -est : est;
    }
    rh = 1.0;
    rl = 0.0;
    er = 0;
    bh = m;
    bl = 0.0;
    eb = kx;
    for (;;) {
        if (n & 1) {
            DDMUL(rh, rl, bh, bl, rh, rl, er)
            er += eb;
        }
        n >>= 1;
        if (n == 0)
            break;
        eb += eb;
        DDMUL(bh, bl, bh, bl, bh, bl, eb)
    }
    if (y > 0) {
        est = kscale(rh, er);
    } else {
        t = 1.0 / rh;
        t -= t * (rl * t);
        est = kscale(t, -er);
    }
    return neg ? -est : est;
}
