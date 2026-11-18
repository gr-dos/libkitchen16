/* Failure reporting for the programs of tests/fpc: only testio's output, so
 * that a program links nothing from libc16 but what it itself uses. Each
 * failure is one line "not ok TAG ..."; at most MAXPRINT lines are printed,
 * all are counted, and done_report() prints "ok" (or the count) and returns
 * the exit status. */

#ifndef FPC_REPORT_H
#define FPC_REPORT_H

#include "testio.h"

#ifndef MAXPRINT
#define MAXPRINT 12
#endif

static unsigned nfail;
static unsigned nchecks;

/* the start of a failure line; returns 0 if it is not to be printed */
static int begin_fail(const char *tag)
{
    nfail++;
    if (nfail > MAXPRINT)
        return 0;
    t_puts("not ok ");
    t_puts(tag);
    t_puts(": ");
    return 1;
}

static int done_report(void)
{
    if (nfail == 0 && nchecks != 0) {
        t_puts("ok\r\n");
        return 0;
    }
    if (nchecks == 0)
        t_puts("not ok: nothing was checked\r\n");
    else {
        t_putu(nfail);
        t_puts(" failures\r\n");
    }
    return 1;
}

#endif
