/* A program that calls strtod and does no floating-point operation links no
 * -fpc helper: the result is assembled from its bits, and is only stored
 * and compared as words. tests/conv/members lists what it links. */

#include <kitchen16.h>
#include "check.h"

int main(void)
{
    union {
        double d;
        unsigned short w[4];
    } u;
    char *end;
    static const char s[] = "-12.5e-1xyz";

    u.d = strtod(s, &end);
    CHECK(u.w[3] == 0xBFF4 && u.w[2] == 0 && u.w[1] == 0 && u.w[0] == 0);
    CHECK(end == s + 8);
    u.d = strtod("1e400", 0);
    CHECK(u.w[3] == 0x7FF0 && u.w[2] == 0 && u.w[1] == 0 && u.w[0] == 0);
    return done();
}
