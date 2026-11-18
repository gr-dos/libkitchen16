/* kscale.h - x * 2^n on the bits (included by k_scale.c and by ldexp.c, so
 * that ldexp links nothing else). Exact except for a result in the
 * subnormal range, which is rounded to nearest, ties to even. An infinity,
 * a NaN and a zero are returned as they are. */

#include "k.h"

#define KW_CLR
#define KW_TOP
#define KW_SHL
#define KW_SHR
#define KW_INC
#include "kw.h"

static double kscale_bits(double x, int n)
{
    kdbl u;
    word m[4];
    int be, k, sticky, rb;

    u.d = x;
    be = K_BE(u);
    if (be == 0x7FF || K_ISZERO(u))
        return x;
    m[0] = u.w[0];
    m[1] = u.w[1];
    m[2] = u.w[2];
    m[3] = u.w[3] & 0x000F;
    if (be) {
        m[3] |= 0x0010;
    } else {
        be = 1;
        k = 52 - kw_top(m, 4);          /* bring the leading bit to bit 52 */
        kw_shl(m, 4, k);
        be -= k;
    }
    if (n > 4000)
        n = 4000;
    if (n < -4000)
        n = -4000;
    be += n;
    if (be >= 0x7FF) {
        K_SETINF(u, u.w[3] & 0x8000);
        return u.d;
    }
    if (be >= 1) {
        m[3] += (word)((be - 1) << 4);  /* the leading bit adds the 1 */
        u.w[0] = m[0];
        u.w[1] = m[1];
        u.w[2] = m[2];
        u.w[3] = (u.w[3] & 0x8000) | m[3];
        return u.d;
    }
    /* subnormal or zero: shift right by 1 - be, rounding */
    k = 1 - be;
    if (k > 60) {
        sticky = 1;
        kw_clr(m, 4);
        rb = 0;
    } else {
        sticky = kw_shr(m, 4, k - 1);
        rb = m[0] & 1;
        kw_shr(m, 4, 1);
    }
    if (rb && (sticky || (m[0] & 1)))
        kw_inc(m, 4);
    u.w[0] = m[0];
    u.w[1] = m[1];
    u.w[2] = m[2];
    u.w[3] = (u.w[3] & 0x8000) | m[3];
    return u.d;
}
