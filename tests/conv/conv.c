/* strtod against a data file (argument; tests/conv/README.txt): the result's
 * bits, the endptr offset and whether errno is ERANGE. errno is zero before
 * each call. The result is compared as bits, so that the program does no
 * floating-point operation. Prints only the failing lines, then "ok". */

#include <errno.h>
#include <kitchen16.h>
#include "lib/data.h"

int main(int argc, char **argv)
{
    struct dline l;
    union dbl u;
    char *end;
    unsigned off;
    int err;

    if (argc != 2 || data_open(argv[1]))
        return 2;
    while (data_next(&l)) {
        end = 0;
        errno = 0;
        u.d = strtod(l.str, &end);
        err = errno;
        off = (unsigned)(end - l.str);
        if (!data_same(&l, u.w) || off != l.off || err != (l.erange ? ERANGE : 0))
            data_fail(&l, u.w, off, err);
    }
    return data_done();
}
