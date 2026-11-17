/* Calls vsnprintf and nothing else of the family: the members are the
 * engine and vsnprintf (tests/fmt/members). */

#include <kitchen16.h>
#include "check.h"

static int wrap(char *s, size_t n, const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vsnprintf(s, n, f, ap);
}

int main(void)
{
    char buf[8];

    CHECK(wrap(buf, sizeof buf, "%d", 12) == 2);
    return done();
}
