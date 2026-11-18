/* ksqrt: the square root, correctly rounded, by an exact integer square
 * root (digit by digit) of the mantissa; no floating-point helper. */

#include "k.h"

#define KW_CLR
#define KW_CPY
#define KW_NZ
#define KW_TOP
#define KW_SHL
#define KW_SHR
#define KW_CMP
#define KW_SUB
#define KW_INC
#include "kw.h"

double ksqrt(double x)
{
    kdbl u;
    word m[4], xx[8], root[5], rem[5], tr[5];
    int be, e, k, q, i, up, low;

    u.d = x;
    be = K_BE(u);
    if (be == 0x7FF) {
        if (K_FRAC0(u)) {
            K_QUIET(u);
            return u.d;
        }
        if (K_SIGN(u)) {
            K_SETNAN(u);
            return u.d;
        }
        return x;
    }
    if (K_ISZERO(u))
        return x;
    if (K_SIGN(u)) {
        K_SETNAN(u);
        return u.d;
    }
    m[0] = u.w[0];
    m[1] = u.w[1];
    m[2] = u.w[2];
    m[3] = u.w[3] & 0x000F;
    if (be) {
        m[3] |= 0x0010;
        e = be - 1075;
    } else {
        e = -1074;
    }
    k = 63 - kw_top(m, 4);
    kw_shl(m, 4, k);                    /* x = m * 2^e, m in [2^63, 2^64) */
    e -= k;
    kw_clr(xx, 8);
    kw_cpy(xx + 4, m, 4);               /* m * 2^64 */
    if (e & 1) {
        kw_shr(xx, 8, 1);               /* m * 2^63 */
        q = (e - 63) / 2;
    } else {
        q = (e - 64) / 2;
    }
    /* root = floor(sqrt(xx)), 64 digits */
    kw_clr(root, 5);
    kw_clr(rem, 5);
    for (i = 63; i >= 0; i--) {
        kw_shl(rem, 5, 2);
        rem[0] |= (xx[i >> 3] >> ((i & 7) * 2)) & 3;
        kw_cpy(tr, root, 5);
        kw_shl(tr, 5, 2);
        tr[0] |= 1;
        kw_shl(root, 5, 1);
        if (kw_cmp(rem, tr, 5) >= 0) {
            kw_sub(rem, tr, 5);
            root[0] |= 1;
        }
    }
    if (!(root[3] & 0x8000)) {          /* 63 bits: normalise */
        kw_shl(root, 4, 1);
        q--;
    }
    /* value = root * 2^q, root in [2^63, 2^64): keep the top 53 bits */
    low = root[0] & 0x07FF;
    kw_shr(root, 4, 11);
    up = low > 0x0400 || (low == 0x0400 && (kw_nz(rem, 5) || (root[0] & 1)));
    if (up)
        kw_inc(root, 4);
    u.w[0] = root[0];
    u.w[1] = root[1];
    u.w[2] = root[2];
    u.w[3] = root[3] + (word)((q + 63 + 1023 - 1) << 4);
    return u.d;
}
