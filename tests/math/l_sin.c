/* Selective linking of sin (docs/spec/math.md, "Size and linking"): a program
 * that calls sin and nothing else of <math.h>. tests/math/check-link looks at
 * the members it links. */

#include <math.h>
#include "check.h"

typedef union {
    double d;
    unsigned short w[4];
} dbl;

int main(void)
{
    dbl x, y, r;

    x.w[0] = 0; x.w[1] = 0; x.w[2] = 0; x.w[3] = 0x3FF0;
    y.w[0] = 0; y.w[1] = 0; y.w[2] = 0; y.w[3] = 0;
    r.d = sin(x.d);
    /* sin(1) = 0.84147098480789650665 = 3FEAED548F090CEE, within 4 ulps */
    CHECK(r.w[3] == 0x3FEA && r.w[2] == 0xED54 && r.w[1] == 0x8F09);
    return done();
}
