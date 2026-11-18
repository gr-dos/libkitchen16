/* kexp: e^a for a double-double argument a = ah + al, |a| <= 745, as 2^n
 * times a double-double y. Reduction by ln 2 in three parts, then a
 * polynomial for e^r - 1 with |r| <= ln 2 / 2, whose leading terms
 * r + r^2/2 are kept in double-double. */

#include "k.h"
#include "kc.h"
#include "kdd.h"

int kexp(double ah, double al, double *y)
{
    static const double Q[13] = KC_EXPQ;
    double nd, t, rh, rl, z, q, s;
    int n, i;

    nd = ah * KC_INVLN2;
    n = (int)(nd < 0.0 ? nd - 0.5 : nd + 0.5);
    nd = (double)n;
    t = ah - nd * KC_LN2A;              /* exact */
    z = nd * KC_LN2B;                   /* exact */
    z = -z;
    rh = kd_sum(t, z, &rl);
    rl += al - nd * KC_LN2C;
    t = kd_sum(rh, rl, &z);
    rh = t;
    rl = z;
    /* p = e^rh - 1 = rh + rh^2/2 + rh^3 Q1(rh), Q1 = (Q - 1/2) / rh */
    q = Q[12];
    for (i = 11; i >= 1; i--)
        q = q * rh + Q[i];
    z = kd_prod(rh, rh, &t);            /* rh^2 = z + t */
    s = kd_sum(rh, 0.5 * z, &nd);       /* rh + rh^2/2 = s + nd */
    nd = nd + (0.5 * t + (z * rh) * q);
    nd += rl + rl * s;                  /* the correction of rl */
    y[0] = kd_sum(1.0, s, &y[1]);
    y[1] += nd;
    K_FASTSUM(y[0], y[1], t, z)
    y[0] = t;
    y[1] = z;
    return n;
}
