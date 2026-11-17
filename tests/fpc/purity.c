/* A program that adds two doubles links nothing from libc16 but its
 * startup (docs/spec/fpc.md, "Must": the members of fpc link nothing from
 * libc16). It uses only testio and one helper; tests/fpc/members, which names
 * the members it links, says which. */

#include "check.h"

static volatile double a, b, r;

int main(void)
{
    volatile unsigned short *w = (volatile unsigned short *)&a;

    w[0] = 0; w[1] = 0; w[2] = 0; w[3] = 0x3FF0;    /* 1.0 */
    w = (volatile unsigned short *)&b;
    w[0] = 0; w[1] = 0; w[2] = 0; w[3] = 0x4000;    /* 2.0 */
    r = a + b;
    w = (volatile unsigned short *)&r;
    CHECK(w[0] == 0 && w[1] == 0 && w[2] == 0 && w[3] == 0x4008);   /* 3.0 */
    return done();
}
