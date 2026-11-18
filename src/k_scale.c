/* kscale: x * 2^n on the bits, for exp, cosh, sinh and pow. */

#include "kscale.h"

double kscale(double x, int n)
{
    return kscale_bits(x, n);
}
