/* sprintf with double conversions, the doubles made from their bits and
 * never operated on: the program, and so the members it links, must
 * contain no floating-point helper (tests/fmt/members). */

#include <kitchen16.h>
#include "check.h"

static char buf[100];

static union {
    unsigned short w[4];
    double d;
} u;

static double bits(unsigned w3, unsigned w2, unsigned w1, unsigned w0)
{
    u.w[3] = w3;
    u.w[2] = w2;
    u.w[1] = w1;
    u.w[0] = w0;
    return u.d;
}

static int same(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

int main(void)
{
    /* 0.1, 2.5, -0, +inf */
    CHECK(sprintf(buf, "%.20g|%.0f|%e", bits(0x3FB9, 0x9999, 0x9999, 0x999A), bits(0x4004, 0, 0, 0), bits(0x8000, 0, 0, 0))
          == 38);
    CHECK(same(buf, "0.10000000000000000555|2|-0.000000e+00"));
    CHECK(sprintf(buf, "%f %G", bits(0x7FF0, 0, 0, 0), bits(0xFFF8, 0, 0, 0)) == 8);
    CHECK(same(buf, "inf -NAN"));
    return done();
}
