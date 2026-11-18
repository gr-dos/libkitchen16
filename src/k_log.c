/* klog: ln x for finite x > 0 as a double-double. x = m 2^k with m in
 * [sqrt(2)/2, sqrt(2)); ln m = 2 atanh(s), s = (m-1)/(m+1) carried as a
 * double-double, the series after 2s with its first term in double-double
 * (|s| <= 0.1716). k ln 2 is added from three parts of ln 2. */

#include "k.h"
#include "kc.h"
#include "kdd.h"

/* ln m for m in [0.707, 1.414] as r[0] + r[1] */
static void klogm(double m, double *r)
{
    static const double P[7] = KC_LOGP;
    double f, dh, dl, sh, sl, z, zl, a, b;
    int i;

    f = m - 1.0;                        /* exact */
    dh = kd_sum(2.0, f, &dl);           /* 2 + f */
    sh = f / dh;
    a = kd_prod(sh, dh, &b);
    sl = (((f - a) - b) - sh * dl) / dh;/* the residual of f = s (2 + f), over dh */
    z = kd_prod(sh, sh, &zl);           /* sh^2 */
    a = kd_prod(sh, z, &b);             /* sh^3 + 3 sh^2 sl = (a, b + 3 z sl + sh zl) */
    b = b + (sh * zl + 3.0 * z * sl);
    zl = a / 3.0;                       /* (2/3) w = 2 (w / 3) */
    dh = kd_prod(zl, 3.0, &f);
    dh = (((a - dh) - f) + b) / 3.0;
    /* s z^2 P(z) */
    dl = P[6];
    for (i = 5; i >= 0; i--)
        dl = dl * z + P[i];
    dl = dl * (sh * z * z);
    /* 2 sh + 2 zl + (2 sl + 2 dh + dl) */
    a = kd_sum(2.0 * sh, 2.0 * zl, &b);
    b = b + (2.0 * sl + 2.0 * dh + dl);
    r[0] = a + b;
    r[1] = b - (r[0] - a);
}

void klog(double x, double *r)
{
    kdbl u;
    double m, a, b, kd;
    int k, be;

    u.d = x;
    be = K_BE(u);
    k = 0;
    if (be == 0) {                      /* subnormal: scale by 2^54 */
        u.d = x * 18014398509481984.0;
        be = K_BE(u);
        k = -54;
    }
    k += be - 1023;
    u.w[3] = (u.w[3] & 0x800F) | 0x3FF0;
    m = u.d;                            /* [1, 2) */
    if (m > 1.4142135623730951) {
        u.w[3] = (u.w[3] & 0x800F) | 0x3FE0;
        m = u.d;                        /* (0.707, 1) */
        k++;
    }
    klogm(m, r);
    if (k == 0)
        return;
    kd = (double)k;
    a = kd_sum(kd * KC_LN2A, r[0], &b); /* exact products: k < 2^11 */
    m = kd_sum(a, kd * KC_LN2B, &a);
    a += (b + r[1]) + kd * KC_LN2C;
    r[0] = m + a;
    r[1] = a - (r[0] - m);
}
