/* fmod (C89 7.5.6.4): exact, bit by bit on the mantissas; no helper. */

#include <errno.h>
#include "k.h"

#define KW_CLR
#define KW_NZ
#define KW_TOP
#define KW_SHL
#define KW_CMP
#define KW_SUB
#include "kw.h"

/* the integer mantissa (with its implicit bit) of finite x and the exponent
 * of its least significant bit */
static int unpack(const kdbl *u, word *m)
{
    int be = K_BE(*u);

    m[0] = u->w[0];
    m[1] = u->w[1];
    m[2] = u->w[2];
    m[3] = u->w[3] & 0x000F;
    if (be) {
        m[3] |= 0x0010;
        return be - 1075;
    }
    return -1074;
}

double __cdecl fmod(double x, double y)
{
    kdbl ux, uy, r;
    word mx[4], my[4], rem[4];
    int ex, ey, n, i, t, sign, cmp;

    ux.d = x;
    uy.d = y;
    if (K_ISNAN(ux)) {
        K_QUIET(ux);
        return ux.d;
    }
    if (K_ISNAN(uy)) {
        K_QUIET(uy);
        return uy.d;
    }
    sign = K_SIGN(ux);
    if (K_ISINF(ux) || K_ISZERO(uy)) {
        errno = EDOM;
        K_SETNAN(r);
        return r.d;
    }
    if (K_ISZERO(ux) || K_ISINF(uy))
        return x;
    ux.w[3] &= 0x7FFF;
    uy.w[3] &= 0x7FFF;
    /* |x| < |y|: x */
    for (i = 3, cmp = 0; i >= 0 && cmp == 0; i--)
        if (ux.w[i] != uy.w[i])
            cmp = ux.w[i] < uy.w[i] ? -1 : 1;
    if (cmp < 0)
        return x;
    ex = unpack(&ux, mx);
    ey = unpack(&uy, my);
    n = ex - ey;                        /* >= 0 here */
    /* rem = mx * 2^n mod my, one bit at a time */
    kw_clr(rem, 4);
    t = kw_top(mx, 4);
    for (i = t; i >= 0; i--) {
        kw_shl(rem, 4, 1);
        rem[0] |= (mx[i >> 4] >> (i & 15)) & 1;
        if (kw_cmp(rem, my, 4) >= 0)
            kw_sub(rem, my, 4);
    }
    for (; n > 0; n--) {
        kw_shl(rem, 4, 1);
        if (kw_cmp(rem, my, 4) >= 0)
            kw_sub(rem, my, 4);
    }
    if (!kw_nz(rem, 4)) {
        K_SETZERO(r, sign);
        return r.d;
    }
    /* result = rem * 2^ey */
    t = kw_top(rem, 4);
    if (t + ey + 1023 >= 1) {
        kw_shl(rem, 4, 52 - t);
        r.w[0] = rem[0];
        r.w[1] = rem[1];
        r.w[2] = rem[2];
        r.w[3] = (word)((t + ey + 1023) << 4) | (rem[3] & 0x000F);
    } else {
        kw_shl(rem, 4, ey + 1074);      /* a subnormal: ey >= -1074 */
        r.w[0] = rem[0];
        r.w[1] = rem[1];
        r.w[2] = rem[2];
        r.w[3] = rem[3];
    }
    r.w[3] |= sign ? 0x8000 : 0;
    return r.d;
}
