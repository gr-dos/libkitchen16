/* atan2, pow and fmod: within 4 ulps (fmod exact), and errno.
 * Usage: binary FILE (a data file of tests/math/data; tests/math/cases). */

#define WANT_F2
#include "lib/mathio.h"

int main(int argc, char **argv)
{
    return driver(argc, argv, 0, 0);
}
