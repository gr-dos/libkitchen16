/* strtod and atof never clear errno, and set it to ERANGE exactly when the
 * data line says so: before each call errno holds a sentinel that is not
 * zero (a value no function here sets, or ERANGE itself on every other line,
 * to show that a line without a range error leaves ERANGE alone too). */

#include <errno.h>
#include <kitchen16.h>
#include "lib/data.h"

#define SENTINEL 4242

int main(int argc, char **argv)
{
    struct dline l;
    union dbl u;
    char *end;
    int before;
    int want;
    int err;
    unsigned n = 0;

    if (argc != 2 || data_open(argv[1]))
        return 2;
    while (data_next(&l)) {
        before = n++ % 2 ? ERANGE : SENTINEL;
        want = l.erange ? ERANGE : before;
        end = 0;
        errno = before;
        u.d = strtod(l.str, &end);
        err = errno;
        if (err != want || !data_same(&l, u.w))
            data_fail(&l, u.w, (unsigned)(end - l.str), err);
        errno = before;
        u.d = strtod(l.str, 0);
        err = errno;
        if (err != want)
            data_fail_msg(&l, "strtod with a null endptr changed errno differently");
        errno = before;
        u.d = atof(l.str);
        err = errno;
        if (err != want || !data_same(&l, u.w))
            data_fail_msg(&l, "atof: wrong errno or result");
    }
    return data_done();
}
