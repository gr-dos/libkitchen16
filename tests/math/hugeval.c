/* HUGE_VAL is +inf (docs/spec/math.md): the bits of its object, compared
 * without any floating-point operation, so that the program links no
 * helper. */

#include <math.h>
#include "check.h"

int main(void)
{
    const unsigned short *w = (const unsigned short *)&HUGE_VAL;

    CHECK(w[0] == 0 && w[1] == 0 && w[2] == 0 && w[3] == 0x7FF0);
    return done();
}
