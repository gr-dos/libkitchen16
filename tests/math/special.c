/* frexp, modf and ldexp, exact, their exponent or integral part too, and errno.
 * Usage: special FILE (a data file of tests/math/data; tests/math/cases). */

#define WANT_SPECIAL
#include "lib/mathio.h"

int main(int argc, char **argv)
{
    return driver(argc, argv, 0, 0);
}
