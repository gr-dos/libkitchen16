/* Calls vfprintf and nothing else of the family: the members are the
 * engine and vfprintf, and libc16's stream code (tests/fmtio/members). */

#include <kitchen16.h>
#include "check.h"

static int wrap(FILE *fp, const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vfprintf(fp, f, ap);
}

int main(void)
{
    CHECK(wrap(stdout, "") == 0);
    return done();
}
