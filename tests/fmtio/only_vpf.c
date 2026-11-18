/* Calls vprintf and nothing else of the family: the members are the
 * engine, vfprintf and vprintf, and libc16's stream code
 * (tests/fmtio/members). */

#include <kitchen16.h>
#include "check.h"

static int wrap(const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vprintf(f, ap);
}

int main(void)
{
    CHECK(wrap("") == 0);
    return done();
}
