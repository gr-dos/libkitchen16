/* errno of every function on the special values and errors of docs/spec/math.md:
 * a call with errno cleared leaves exactly the expected value (0, EDOM or
 * ERANGE), and a call that expects no error, made again with errno set to
 * another value, does not touch it (a function never clears errno). The
 * results are checked as in the other programs.
 * Usage: errno FILE (errno.dat). */

#define WANT_F1
#define WANT_F2
#define WANT_SPECIAL
#include "lib/mathio.h"

int main(int argc, char **argv)
{
    return driver(argc, argv, 0, 1);
}
