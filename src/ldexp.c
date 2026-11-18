/* ldexp (C89 7.5.4.3): x * 2^exp, exact in the range; a result in the
 * subnormal range is rounded to nearest even (docs/spec/math.md). Works on
 * the bits and links no helper. */

#include <errno.h>
#include "kscale.h"

double __cdecl ldexp(double x, int exp)
{
    kdbl u, r;

    u.d = x;
    r.d = kscale_bits(x, exp);
    if (K_BE(u) != 0x7FF && !K_ISZERO(u)) {
        if (K_ISINF(r) || K_ISZERO(r))
            errno = ERANGE;
    }
    return r.d;
}
