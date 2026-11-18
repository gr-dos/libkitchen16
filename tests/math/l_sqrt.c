/* Selective linking of sqrt (docs/spec/math.md, "Size and linking"): a program
 * that calls sqrt and nothing else of <math.h>. tests/math/check-link looks at
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

    x.w[0] = 0; x.w[1] = 0; x.w[2] = 0; x.w[3] = 0x4010;
    y.w[0] = 0; y.w[1] = 0; y.w[2] = 0; y.w[3] = 0;
    r.d = sqrt(x.d);
    CHECK(r.w[3] == 0x4000 && (r.w[2] | r.w[1] | r.w[0]) == 0);
    return done();
}
