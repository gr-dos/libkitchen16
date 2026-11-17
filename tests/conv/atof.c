/* atof(s) is strtod(s, 0): the same bits, for the lines of a data file
 * (argument), and errno is set the same way. */

#include <errno.h>
#include <kitchen16.h>
#include "lib/data.h"

int main(int argc, char **argv)
{
    struct dline l;
    union dbl a;
    union dbl s;
    int ea;
    int es;

    if (argc != 2 || data_open(argv[1]))
        return 2;
    while (data_next(&l)) {
        errno = 0;
        a.d = atof(l.str);
        ea = errno;
        errno = 0;
        s.d = strtod(l.str, (char **)0);
        es = errno;
        if (ea != es || !data_same(&l, a.w) || !data_same(&l, s.w))
            data_fail(&l, a.w, 0, ea);
    }
    return data_done();
}
