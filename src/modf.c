/* modf (C89 7.5.4.6): on the bits, no helper. */

#include "kint.h"

#define KW_TOP
#define KW_SHL
#include "kw.h"

double __cdecl modf(double x, double *iptr)
{
    kdbl u, r;
    word frac[4];
    int e, be, t, sign;

    u.d = x;
    be = K_BE(u);
    sign = K_SIGN(u);
    e = be - 1023;
    if (be == 0x7FF) {
        *iptr = x;
        if (K_FRAC0(u)) {
            K_QUIET(u);
            *iptr = u.d;
            return u.d;
        }
        K_SETZERO(r, sign);
        return r.d;
    }
    if (e < 0) {                        /* |x| < 1 */
        K_SETZERO(r, sign);
        *iptr = r.d;
        return x;
    }
    if (e >= 52) {                      /* integral */
        *iptr = x;
        K_SETZERO(r, sign);
        return r.d;
    }
    if (!ktrunc(&u, e, frac)) {
        *iptr = x;
        K_SETZERO(r, sign);
        return r.d;
    }
    *iptr = u.d;
    /* the fraction bits, a value frac * 2^(e - 52), normalised */
    t = kw_top(frac, 4);
    kw_shl(frac, 4, 52 - t);
    r.w[0] = frac[0];
    r.w[1] = frac[1];
    r.w[2] = frac[2];
    r.w[3] = (sign ? 0x8000 : 0) | (word)((e - 52 + t + 1023) << 4) | (frac[3] & 0x000F);
    return r.d;
}
