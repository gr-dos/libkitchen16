/* Calls vsprintf and nothing else of the family: the members are the
 * engine and vsprintf (tests/fmt/members). */

#include <kitchen16.h>
#include "check.h"

static int wrap(char *s, const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vsprintf(s, f, ap);
}

int main(void)
{
    char buf[8];

    CHECK(wrap(buf, "%d", 12) == 2);
    return done();
}
