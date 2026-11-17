/* The corpus runner: fmt FILE [all]. Reads the data file FILE (see
 * lib/driver.h and gen) and, for each line, calls sprintf, vsprintf,
 * snprintf or vsnprintf (every line with a different one in turn, or all
 * four with `all`; N lines call the two n functions) and compares the
 * return value and the whole target buffer. Prints the first failing lines
 * and then ok, or a count of the failures. */

#define DRIVER_MEMCHECK
#include "lib/driver.h"

int main(int argc, char **argv)
{
    FILE *fp;
    struct rec r;
    int rc, all;
    unsigned count = 0;

    CHECK(argc >= 1);
    if (argc < 2) {
        t_puts("usage: fmt FILE [all]\r\n");
        return 2;
    }
    all = argc > 2 && argv[2][0] == 'a';
    fp = fopen(argv[1], "rb");
    if (fp == 0) {
        t_puts("cannot open the data file\r\n");
        return 2;
    }
    while ((rc = next_rec(fp, &r)) != 0) {
        if (rc < 0) {
            fail_head("data");
            t_puts("malformed line\r\n");
            break;
        }
        run_rec(&r, all);
        count++;
    }
    fclose(fp);
    if (count == 0) {
        fail_head("data");
        t_puts("no records\r\n");
    }
    if (failures) {
        t_putu(failures);
        t_puts(" failures in ");
        t_putu(count);
        t_puts(" records\r\n");
    }
    return done();
}
