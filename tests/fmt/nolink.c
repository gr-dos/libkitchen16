/* A program that calls only sprintf (and no conversion at all): it links
 * the engine and sprintf, no stream code of libkitchen16's and none of
 * libc16's on its account (tests/fmt/members). */

#include <kitchen16.h>
#include "check.h"

int main(void)
{
    char buf[16];

    CHECK(sprintf(buf, "x%dy", 5) == 3);
    CHECK(buf[0] == 'x' && buf[1] == '5' && buf[2] == 'y' && buf[3] == 0);
    return done();
}
