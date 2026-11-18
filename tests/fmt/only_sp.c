/* Calls sprintf and nothing else of the family: the members are the
 * engine and sprintf (tests/fmt/members). */

#include <kitchen16.h>
#include "check.h"

int main(void)
{
    char buf[8];

    CHECK(sprintf(buf, "%d", 12) == 2);
    return done();
}
