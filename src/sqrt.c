/* sqrt (C89 7.5.5.2): correctly rounded; a domain error for x < 0. */

#include <errno.h>
#include "k.h"

double __cdecl sqrt(double x)
{
    kdbl u;

    u.d = x;
    if (K_SIGN(u) && !K_ISNAN(u) && !K_ISZERO(u))
        errno = EDOM;
    return ksqrt(x);
}
