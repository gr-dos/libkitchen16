/* katan2: atan t for a double-double t = th + tl >= 0 (+inf included). For
 * t > 1, pi/2 - atan(1/t). On [0, 1] atan t = atan(j/8) + atan u with
 * u = (t - j/8) / (1 + t j/8), |u| <= 1/16, the first from a table in two
 * parts and the second by a polynomial. */

#include "k.h"
#include "kc.h"
#include "kdd.h"

double katan2(double th, double tl)
{
    static const double TH[9] = KC_ATANH;
    static const double TL[9] = KC_ATANL;
    static const double A[7] = KC_ATANA;
    double u, c, z, a, b, d, e;
    int j, inv, i;

    inv = 0;
    if (th > 1.0) {                     /* 1 / t as a double-double */
        inv = 1;
        u = 1.0 / th;
        if (th < 1.0e150) {             /* else 1/t is below 1e-150: no correction */
            c = kd_prod(u, th, &z);
            tl = (((1.0 - c) - z) - u * tl) / th;
        } else {
            tl = 0.0;
        }
        th = u;
    }
    j = (int)(th * 8.0 + 0.5);
    if (j == 0) {
        z = th * th;
        a = A[6];
        for (i = 5; i >= 0; i--)
            a = a * z + A[i];
        u = th + (tl + (th * z) * a);   /* atan t */
        if (!inv)
            return u;
        d = 0.0;
    } else {
        c = (double)j * 0.125;
        a = kd_prod(th, c, &b);
        d = kd_sum(1.0, a, &e);         /* 1 + t c = d + e */
        e += b + tl * c;
        u = ((th - c) + tl) / d;
        u -= u * (e / d);
        z = u * u;
        a = A[6];
        for (i = 5; i >= 0; i--)
            a = a * z + A[i];
        d = TH[j];
        u = u + (TL[j] + (u * z) * a);  /* atan u + the low part of the table value */
        if (!inv)
            return d + u;
    }
    /* pi/2 - (d + u) */
    a = kd_sum(KC_PIO2H, -d, &e);
    return a + (e + (KC_PIO2L - u));
}
