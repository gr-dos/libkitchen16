/* log (C89 7.5.4.4). */

#include <errno.h>
#include "k.h"

double __cdecl log(double x)
{
    kdbl u;
    double r[2];

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    if (K_ISZERO(u)) {
        errno = ERANGE;
        K_SETINF(u, 1);
        return u.d;
    }
    if (K_SIGN(u)) {
        errno = EDOM;
        K_SETNAN(u);
        return u.d;
    }
    if (K_ISINF(u))
        return x;
    klog(x, r);
    return r[0] + r[1];
}
