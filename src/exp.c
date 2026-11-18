/* exp (C89 7.5.4.1): within a few ulps; ERANGE for an overflow and for an
 * underflow to zero. */

#include <errno.h>
#include "k.h"

double __cdecl exp(double x)
{
    kdbl u;
    double y[2];
    int n;

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    if (K_ISINF(u)) {
        if (K_SIGN(u))
            K_SETZERO(u, 0);
        return u.d;
    }
    if (x > 709.782712893384) {
        errno = ERANGE;
        K_SETINF(u, 0);
        return u.d;
    }
    if (x < -745.1332191019412) {
        errno = ERANGE;
        K_SETZERO(u, 0);
        return u.d;
    }
    n = kexp(x, 0.0, y);
    return kscale(y[0], n);
}
