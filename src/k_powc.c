/* kpowc: the cases of pow that need no logarithm: the special values, the
 * integer exponents of at most 1024 (by kpowi), the powers of two and y = 1/2.
 * Returns 0 with the result in *out (errno set), or 1 or 2 when x^y is to be
 * computed as exp(y ln|x|), with |x| in *out and the result positive (1) or
 * negative (2). */

#include <errno.h>
#include "k.h"

/* 0: y is not an integer (or is infinite), 1: even, 2: odd */
static int yclass(const kdbl *uy)
{
    int e = (int)K_BE(*uy) - 1023, fb, i, nb;
    word mask;

    if (e < 0 || K_BE(*uy) == 0x7FF)
        return 0;
    if (e >= 52)
        return (e == 52 && (uy->w[0] & 1)) ? 2 : 1;
    fb = 52 - e;                        /* the bit of weight 1 is bit fb of the mantissa */
    for (i = 0; i < 4; i++) {           /* a fraction bit set: not an integer */
        nb = fb - 16 * i;
        if (nb <= 0)
            break;
        mask = nb >= 16 ? 0xFFFF : (word)((1u << nb) - 1);
        if (uy->w[i] & mask)
            return 0;
    }
    /* the mantissa has its implicit bit at bit 52, which is bit fb only for y = 1 */
    if (fb == 52)
        return 2;
    return ((uy->w[fb >> 4] >> (fb & 15)) & 1) ? 2 : 1;
}

int kpowc(double x, double y, double *out)
{
    kdbl ux, uy, r;
    double ax, ay;
    int sx, sy, yc, neg, k;

    ux.d = x;
    uy.d = y;
    r.d = 1.0;
    if (K_ISZERO(uy) || (ux.w[3] == 0x3FF0 && !(ux.w[2] | ux.w[1] | ux.w[0]))) {
        *out = r.d;                     /* pow(x, 0) and pow(1, y) are 1 */
        return 0;
    }
    if (K_ISNAN(ux)) {
        K_QUIET(ux);
        *out = ux.d;
        return 0;
    }
    if (K_ISNAN(uy)) {
        K_QUIET(uy);
        *out = uy.d;
        return 0;
    }
    sx = K_SIGN(ux);
    sy = K_SIGN(uy);
    yc = yclass(&uy);
    neg = sx && yc == 2;
    if (K_ISINF(uy)) {
        r.d = x;
        r.w[3] &= 0x7FFF;
        ax = r.d;
        if (ax == 1.0) {
            r.d = 1.0;
        } else if ((ax > 1.0) != (sy != 0)) {
            K_SETINF(r, 0);
        } else {
            K_SETZERO(r, 0);
        }
        *out = r.d;
        return 0;
    }
    if (K_ISINF(ux)) {
        if (sy)
            K_SETZERO(r, neg);
        else
            K_SETINF(r, neg);
        *out = r.d;
        return 0;
    }
    if (K_ISZERO(ux)) {
        if (sy) {
            errno = ERANGE;
            K_SETINF(r, neg);
        } else {
            K_SETZERO(r, neg);
        }
        *out = r.d;
        return 0;
    }
    if (sx) {
        if (yc == 0) {
            errno = EDOM;
            K_SETNAN(r);
            *out = r.d;
            return 0;
        }
        ux.w[3] &= 0x7FFF;
    }
    ax = ux.d;
    if (y == 1.0) {
        *out = x;
        return 0;
    }
    uy.w[3] &= 0x7FFF;
    ay = uy.d;
    if (yc != 0 && K_BE(ux) != 0 && !(ux.w[2] | ux.w[1] | ux.w[0] | (ux.w[3] & 0x000F))) {
        /* a power of two, 2^k: 2^(k y) */
        k = (int)K_BE(ux) - 1023;
        ay = (double)k * y;
        if (ay > 4000.0)
            ay = 4000.0;
        if (ay < -4000.0)
            ay = -4000.0;
        ax = kscale(1.0, (int)ay);
    } else if (yc != 0 && ay <= 1024.0) {
        ax = kpowi(ax, (int)y);
    } else if (!yc && y == 0.5) {
        ax = ksqrt(ax);
    } else if (K_BE(uy) > 1023 + 900) { /* |y| > 2^900: x is not 1 */
        if ((ax > 1.0) == (sy == 0))
            K_SETINF(r, 0);
        else
            K_SETZERO(r, 0);
        ax = r.d;
    } else {
        *out = ax;                      /* the logarithm and the exponential */
        return neg ? 2 : 1;
    }
    if (neg)
        ax = -ax;
    r.d = ax;
    if (K_ISINF(r) || K_ISZERO(r))
        errno = ERANGE;
    *out = ax;
    return 0;
}
