/* frexp (C89 7.5.4.2): on the bits, no helper. */

#include "k.h"

#define KW_TOP
#define KW_SHL
#include "kw.h"

double __cdecl frexp(double x, int *exp)
{
    kdbl u;
    word m[4];
    int be, t;

    u.d = x;
    be = K_BE(u);
    if (be == 0x7FF || K_ISZERO(u)) {
        *exp = 0;
        return x;
    }
    if (be) {
        *exp = be - 1022;
        u.w[3] = (u.w[3] & 0x800F) | 0x3FE0;
        return u.d;
    }
    /* subnormal: x = m * 2^-1074 with 2^t <= m < 2^(t+1) */
    m[0] = u.w[0];
    m[1] = u.w[1];
    m[2] = u.w[2];
    m[3] = u.w[3] & 0x000F;
    t = kw_top(m, 4);
    kw_shl(m, 4, 52 - t);               /* the leading bit at bit 52 */
    *exp = t + 1 - 1074;
    u.w[0] = m[0];
    u.w[1] = m[1];
    u.w[2] = m[2];
    u.w[3] = (u.w[3] & 0x8000) | (m[3] & 0x000F) | 0x3FE0;
    return u.d;
}
