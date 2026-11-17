/* Calls printf and nothing else of the family: the members are the
 * engine, vfprintf and printf, and libc16's stream code
 * (tests/fmtio/members). */

#include <kitchen16.h>
#include "check.h"

int main(void)
{
    CHECK(printf("") == 0);
    return done();
}
