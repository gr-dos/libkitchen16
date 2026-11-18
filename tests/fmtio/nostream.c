/* A program that calls sprintf and none of the stream functions links no
 * stream code of libkitchen16's and none of libc16's on account of
 * libkitchen16: no vfprintf, no fwrite, no stdout (tests/fmtio/members). */

#include <kitchen16.h>
#include "check.h"

int main(void)
{
    char buf[32];

    CHECK(sprintf(buf, "%s=%d|%5d", "n", 7, 42) > 0);
    return done();
}
