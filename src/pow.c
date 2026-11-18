/* pow (C89 7.5.5.1): x^y. kpowc settles the special values, the integer
 * exponents (kpowi: exact when the power is a double) and the powers of
 * two; the rest is exp(y ln|x|) with the product in double-double. */

#include <errno.h>
#include "k.h"
#include "kdd.h"

/* w[0] as bits (the stack frame of pow is part of the stack bound, so
 * there is no separate kdbl) */
#define WU (*(kdbl *)w)

double __cdecl pow(double x, double y)
{
    double a, ph, pe, w[2];
    int c, n;

    c = kpowc(x, y, &a);                /* 0: the result is a; else the magnitude |x| */
    if (!c)
        return a;
    klog(a, w);
    ph = kd_prod(y, w[0], &pe);
    pe += y * w[1];
    K_FASTSUM(ph, pe, a, w[0])          /* y ln|x| = a + w[0] */
    if (a > 710.0) {                    /* overflow */
        K_SETINF(WU, 0);
        a = w[0];
    } else if (a < -746.0) {            /* underflow */
        K_SETZERO(WU, 0);
        a = w[0];
    } else {
        n = kexp(a, w[0], w);
        a = kscale(w[0], n);
    }
    if (c == 2)
        a = -a;
    w[0] = a;
    if (K_ISINF(WU) || K_ISZERO(WU))
        errno = ERANGE;
    return a;
}
