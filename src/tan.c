/* tan (C89 7.5.2.7). */

#include <errno.h>
#include "k.h"

double __cdecl tan(double x)
{
    kdbl u;

    u.d = x;
    if (K_ISNAN(u)) {
        K_QUIET(u);
        return u.d;
    }
    if (K_ISINF(u)) {
        errno = EDOM;
        K_SETNAN(u);
        return u.d;
    }
    if (K_ISZERO(u))
        return x;
    return ktrig(x, 2);
}
