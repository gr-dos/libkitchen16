/* the functions of one double (sin cos tan asin acos atan sinh cosh tanh exp log log10 sqrt fabs floor ceil): within their bound in ulps (4; sqrt, fabs, floor and ceil exact), and errno.
 * Usage: unary FILE (a data file of tests/math/data; tests/math/cases). */

#define WANT_F1
#include "lib/mathio.h"

int main(int argc, char **argv)
{
    return driver(argc, argv, 0, 0);
}
