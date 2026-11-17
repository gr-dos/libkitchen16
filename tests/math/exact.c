/* The results that docs/spec/math.md says are exact (f(0) identities, powers
 * of 2 and of other integers, pow(x, 0) and pow(x, 1), log10 of the powers
 * of ten, sqrt of squares, fabs, floor, ceil, fmod): every result must equal
 * the expected one bit for bit, whichever function it is. Usage: exact FILE
 * (exact.dat, p2n.dat, p2p.dat, l10.dat). */

#define WANT_F1
#define WANT_F2
#include "lib/mathio.h"

int main(int argc, char **argv)
{
    return driver(argc, argv, 1, 0);
}
