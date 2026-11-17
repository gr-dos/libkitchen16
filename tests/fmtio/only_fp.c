/* Calls fprintf and nothing else of the family: the members are the
 * engine, vfprintf and fprintf, and the stream code libc16 brings with
 * fwrite (tests/fmtio/members). */

#include <kitchen16.h>
#include "check.h"

int main(void)
{
    CHECK(fprintf(stdout, "") == 0);
    return done();
}
