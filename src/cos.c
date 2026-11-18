/* cos (C89 7.5.2.5). */

#include <errno.h>
#include "k.h"

double __cdecl cos(double x)
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
    return ktrig(x, 1);
}
