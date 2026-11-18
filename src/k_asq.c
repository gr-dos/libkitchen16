/* kasq: t = a / sqrt(1 - a^2) for a double-double a = ah + al with
 * 0 <= a < 1, as t[0] + t[1] (relative error about 2^-100): the argument of
 * atan that gives asin a. */

#include "k.h"

void kasq(double ah, double al, double *t)
{
    double p, e, qh, ql, sh, sl;

    p = kd_prod(ah, ah, &e);            /* a^2 */
    e += 2.0 * ah * al;
    qh = kd_sum(1.0, -p, &ql);          /* 1 - a^2 */
    ql -= e;
    sh = ksqrt(qh);                     /* sqrt(qh + ql) */
    p = kd_prod(sh, sh, &e);
    sl = (((qh - p) - e) + ql) / (2.0 * sh);
    qh = ah / sh;                       /* a / (sh + sl) */
    p = kd_prod(qh, sh, &e);
    t[1] = (((ah - p) - e) + (al - qh * sl)) / sh;
    t[0] = qh;
}
