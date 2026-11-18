/* ktrig: sin, cos and tan of a finite x. The argument is reduced to
 * r = x - N pi/2 as a double-double, |r| <= pi/4: by Cody and Waite's
 * method with pi/2 in four parts for |x| < 2^19 pi/2, and for larger
 * arguments by Payne and Hanek's: the product of the mantissa of x with
 * 1,280 bits of 2/pi, of which a window of 192 bits is used, so that r is
 * right however large x is. Then polynomials for sin and cos on |r| <= pi/4. */

#include "k.h"
#include "kc.h"
#include "kdd.h"

#define KW_CLR
#define KW_CPY
#define KW_TOP
#define KW_SHL
#define KW_SHR
#define KW_MUL
#include "kw.h"

/* sin(rh + rl) for |rh| <= pi/4, as v[0] + v[1] */
static void sink(double rh, double rl, double *v)
{
    static const double S[8] = KC_SINS;
    double z, s, w;
    int i;

    z = rh * rh;
    s = S[7];
    for (i = 6; i >= 0; i--)
        s = s * z + S[i];
    w = (rh * z) * s + (rl - rl * z * 0.5);
    v[0] = kd_sum(rh, w, &v[1]);
}

/* cos(rh + rl), as v[0] + v[1] */
static void cosk(double rh, double rl, double *v)
{
    static const double C[8] = KC_COSC;
    double z, c, d;
    int i;

    z = rh * rh;
    c = C[7];
    for (i = 6; i >= 0; i--)
        c = c * z + C[i];
    d = -(0.5 * z - ((z * z) * c - rh * rl));
    v[0] = kd_sum(1.0, d, &v[1]);
}

/* The double of the integer in a[0..3] (at most 53 significant bits)
 * times 2^e2. */
static double mkd(word *a, int e2)
{
    kdbl u;
    int t = kw_top(a, 4);

    if (t < 0)
        return 0.0;
    if (t > 52)
        kw_shr(a, 4, t - 52);
    else
        kw_shl(a, 4, 52 - t);
    u.w[0] = a[0];
    u.w[1] = a[1];
    u.w[2] = a[2];
    u.w[3] = (word)((t + e2 + 1023) << 4) | (a[3] & 0x000F);
    return u.d;
}

/* The fraction g * 2^-k (g in p[0..15], below 2^k, its leading bit at tb) as
 * a double-double r = (hi + lo) pi/2 */
static void frac2r(word *p, int tb, int k, double *rh, double *rl)
{
    word a[4];
    int e0;
    double hi, lo, ph, pe;

    kw_shl(p, 16, 255 - tb);            /* leading bit at 255: p[15..12] is a 64-bit chunk */
    e0 = tb - k;                        /* the exponent of the leading bit */
    /* hi = top 53 bits of p[15..12]: (c1 >> 11) 2^(e0 - 52) */
    kw_cpy(a, p + 12, 4);
    kw_shr(a, 4, 11);
    hi = mkd(a, e0 - 52);
    /* lo = ((c1 & 0x7FF) << 42 | c2 >> 22) 2^(e0 - 105), c2 = p[11..8] */
    kw_cpy(a, p + 8, 4);
    kw_shr(a, 4, 22);
    a[2] |= (word)((p[12] & 0x07FF) << 10);
    a[3] |= (word)((p[12] & 0x07FF) >> 6);
    lo = mkd(a, e0 - 105);
    ph = kd_prodc(hi, KC_PIO2H, KC_PIO2H_H, KC_PIO2H_L, &pe);
    pe += hi * KC_PIO2L + lo * KC_PIO2H;
    K_FASTSUM(ph, pe, *rh, *rl)
}

/* p = m * w, the product of the mantissa of the large positive x with a
 * window of 192 bits of 2/pi; returns k, the position of the binary point in
 * p (bits up from the bottom): the bits of p below it are the fraction of
 * x 2/pi */
static int ph_mul(const kdbl *ux, word *p)
{
    static const word T[KC_TWOPI_WORDS] = KC_TWOPI;
    word m[4], w[12];
    unsigned long c;
    int e, j0, i0, i, j;

    m[0] = ux->w[0];
    m[1] = ux->w[1];
    m[2] = ux->w[2];
    m[3] = (ux->w[3] & 0x000F) | 0x0010;
    e = (int)K_BE(*ux) - 1075;          /* x = m 2^e */
    /* the bits i >= e - 1 of 2/pi = sum b_i 2^-i matter: a window of 12
     * words from the word that holds bit max(e - 1, 1) */
    j0 = (e - 1 < 1 ? 0 : e - 2) >> 4;
    i0 = 16 * j0 + 1;
    for (i = 0; i < 12; i++)
        w[11 - i] = T[j0 + i];
    kw_clr(p, 16);
    for (i = 0; i < 4; i++) {
        c = 0;
        for (j = 0; j < 12; j++) {
            c += mul16(m[i], w[j]) + p[i + j];
            p[i + j] = (word)c;
            c >>= 16;
        }
        p[i + 12] = (word)c;
    }
    return i0 + 191 - e;
}

/* r = x - N pi/2 by Payne and Hanek for a large positive x; returns N mod 4
 * and r as (*rh, *rl). */
static int reduce_big(const kdbl *ux, double *rh, double *rl)
{
    word p[16];
    unsigned long c;
    int i, k, q, neg, tb;

    k = ph_mul(ux, p);
    q = ((p[k >> 4] >> (k & 15)) & 1) | (((p[(k + 1) >> 4] >> ((k + 1) & 15)) & 1) << 1);
    /* the fraction: the low k bits of p */
    for (i = k; i < 256; i++)
        p[i >> 4] &= (word)~(1u << (i & 15));
    neg = (p[(k - 1) >> 4] >> ((k - 1) & 15)) & 1;
    if (neg) {                          /* >= 1/2: the next integer, p = 2^k - p */
        c = 1;
        for (i = 0; i < 16; i++) {
            c += (word)~p[i];
            p[i] = (word)c;
            c >>= 16;
        }
        for (i = k; i < 256; i++)
            p[i >> 4] &= (word)~(1u << (i & 15));
        q = (q + 1) & 3;
    }
    tb = kw_top(p, 16);
    if (tb < 0) {
        *rh = 0.0;
        *rl = 0.0;
        return q;
    }
    frac2r(p, tb, k, rh, rl);
    if (neg) {
        *rh = -*rh;
        *rl = -*rl;
    }
    return q;
}

/* sin (mode 0), cos (1) or tan (2) of N pi/2 + r, N mod 4 = q; for sin and tan
 * of x < 0 the caller changes the sign */
static double kern(int q, int mode, double rh, double rl)
{
    double sv[2], cv[2], res, p, e;

    if (mode == 2) {
        sink(rh, rl, sv);
        cosk(rh, rl, cv);
        if (q & 1) {                    /* -cos / sin */
            res = sv[0];
            sv[0] = -cv[0];
            cv[0] = res;
            res = sv[1];
            sv[1] = -cv[1];
            cv[1] = res;
        }
        /* sv / cv: the quotient and a correction from the exact remainder */
        res = sv[0] / cv[0];
        p = kd_prod(res, cv[0], &e);
        return res + (((sv[0] - p) - e) + (sv[1] - res * cv[1])) / cv[0];
    }
    if (mode == 1)
        q += 1;                         /* cos x = sin(x + pi/2) */
    if (q & 1)
        cosk(rh, rl, cv);
    else
        sink(rh, rl, cv);
    res = cv[0] + cv[1];
    return (q & 2) ? -res : res;
}

double ktrig(double x, int mode)
{
    kdbl u;
    double ax, rh, rl, nd, t, z;
    int q, neg;
    long n;

    u.d = x;
    neg = K_SIGN(u);
    u.w[3] &= 0x7FFF;
    ax = u.d;
    if (ax <= 0.7853981633974483) {
        q = 0;
        rh = ax;
        rl = 0.0;
    } else if (ax < 823549.0) {
        nd = ax * KC_INVPIO2;
        n = (long)(nd + 0.5);
        nd = (double)n;
        q = (int)(n & 3);
        t = ax - nd * KC_PP1;           /* exact */
        z = -(nd * KC_PP2);
        rh = kd_sum(t, z, &rl);
        rl -= nd * KC_PP3;
        rl -= nd * KC_PP4;
        t = kd_sum(rh, rl, &z);
        rh = t;
        rl = z;
    } else {
        q = reduce_big(&u, &rh, &rl);
    }
    t = kern(q, mode, rh, rl);
    return (neg && mode != 1) ? -t : t;
}
