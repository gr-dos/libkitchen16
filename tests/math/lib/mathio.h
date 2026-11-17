/* The common part of the <math.h> test programs (the .c files of tests/math): they read
 * a data file of tests/math/data (written by tests/math/gen; its line format
 * is described there), call the function on each line, and print only the
 * lines that fail, then "ok".
 *
 * Nothing here uses floating point operations: the arguments and results
 * are 16-bit words (a double is four of them, least significant first, the
 * memory order of the 8086) that are only copied to and from the function
 * through a union, parsed from hex by hand and compared as integers. The
 * error of an inexact result is the difference of the integer bit patterns
 * (one unit is one unit in the last place), computed with word arithmetic.
 *
 * A program defines WANT_F1, WANT_F2 and/or WANT_SPECIAL before including
 * this to get the rows of the functions it tests (so that it links nothing
 * else), and calls driver() with the options it wants:
 *
 *   exact:  every result must equal the expected one bit for bit
 *   preset: a row that expects errno 0 is called a second time with errno
 *           set to another value, which the call must leave alone
 *
 * The line format, and the rules a result must meet:
 *   - an expected NaN is bit-for-bit, except "nan", for which any NaN will do;
 *   - an expected zero or infinity is bit-for-bit (the sign of a zero counts);
 *   - any other result has the sign of the expected one and is within the
 *     function's bound in ulps (0 for the exact functions), and is not 0,
 *     infinity or NaN.
 */

#ifndef MATHIO_H
#define MATHIO_H

#include <errno.h>
#include <stdio.h>
#include <math.h>
#include "testio.h"

typedef unsigned short word;

typedef union {
    double d;
    word w[4];                  /* w[3] has the sign and the exponent */
} dbl;

#define SENTINEL 12345          /* an errno value no function sets */
#define MAXFAIL 20              /* failing lines printed */

static unsigned lineno;
static unsigned nrows;
static unsigned nfail;
static int g_exact;
static int g_preset;

/* ---- output ---- */

static void put_hex(const word *w)
{
    char buf[17];
    int i;

    for (i = 0; i < 16; i++) {
        word v = w[3 - i / 4];
        v = (v >> (12 - 4 * (i % 4))) & 15;
        buf[i] = v < 10 ? '0' + v : 'A' + v - 10;
    }
    buf[16] = '\0';
    t_puts(buf);
}

static void put_int(int v)
{
    if (v < 0) {
        t_puts("-");
        t_putu(-(unsigned)v);
    } else {
        t_putu(v);
    }
}

static void begin_fail(const char *name)
{
    nfail++;
    if (nfail <= MAXFAIL) {
        t_puts("line ");
        t_putu(lineno);
        t_puts(": ");
        t_puts(name);
    }
}

static void end_fail(void)
{
    if (nfail <= MAXFAIL)
        t_puts("\r\n");
}

static void fail_text(const char *name, const char *why)
{
    begin_fail(name);
    if (nfail <= MAXFAIL) {
        t_puts(" ");
        t_puts(why);
    }
    end_fail();
}

/* ---- comparison ---- */

static int is_nan(const word *w)
{
    return (w[3] & 0x7FF0) == 0x7FF0 && ((w[3] & 0x000F) | w[2] | w[1] | w[0]) != 0;
}

static int is_zero(const word *w)
{
    return (w[3] & 0x7FFF) == 0 && (w[2] | w[1] | w[0]) == 0;
}

static int is_inf(const word *w)
{
    return (w[3] & 0x7FFF) == 0x7FF0 && (w[2] | w[1] | w[0]) == 0;
}

static int same(const word *a, const word *b)
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

/* |a - b| of two magnitudes (the sign bits cleared), as a count of units
 * in the last place; 65535 stands for that or more. */
static unsigned distance(const word *a, const word *b)
{
    word x[4], y[4], d[4];
    unsigned borrow = 0, t;
    int i;

    for (i = 0; i < 4; i++) {
        x[i] = a[i];
        y[i] = b[i];
    }
    x[3] &= 0x7FFF;
    y[3] &= 0x7FFF;
    /* swap so that x >= y */
    for (i = 3; i >= 0; i--) {
        if (x[i] != y[i]) {
            if (x[i] < y[i]) {
                word s[4];
                int j;
                for (j = 0; j < 4; j++) {
                    s[j] = x[j];
                    x[j] = y[j];
                    y[j] = s[j];
                }
            }
            break;
        }
    }
    for (i = 0; i < 4; i++) {
        t = (unsigned)x[i] - y[i] - borrow;
        borrow = (x[i] < y[i] || (x[i] == y[i] && borrow)) ? 1 : 0;
        d[i] = t;
    }
    if (d[3] | d[2] | d[1])
        return 65535u;
    return d[0];
}

/* Whether a result meets the rules above; *ulps is the error found, 0 when
 * the comparison is bit for bit. */
static int result_ok(const word *got, const word *want, int want_nan_any, unsigned bound, unsigned *ulps)
{
    *ulps = 0;
    if (want_nan_any)
        return is_nan(got);
    if (is_nan(want) || is_zero(want) || is_inf(want))
        return same(got, want);
    if (is_nan(got) || is_zero(got) || is_inf(got) || ((got[3] ^ want[3]) & 0x8000)) {
        *ulps = 65535u;
        return 0;
    }
    *ulps = distance(got, want);
    return *ulps <= bound;
}

/* ---- parsing ---- */

static char *skip(char *p)
{
    while (*p == ' ')
        p++;
    return p;
}

static int is_end(const char *p)
{
    return *p == '\0' || *p == '\r' || *p == '\n';
}

/* The next word of the line into name (at most 15 characters). */
static char *get_name(char *p, char *name)
{
    int n = 0;

    p = skip(p);
    while (*p != ' ' && !is_end(p)) {
        if (n < 15)
            name[n++] = *p;
        p++;
    }
    name[n] = '\0';
    return p;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

/* 16 hex digits into w (w[3] first), or "nan" (any NaN, *any set); NULL
 * if there is neither. */
static char *get_bits(char *p, word *w, int *any)
{
    int i;

    p = skip(p);
    *any = 0;
    if (p[0] == 'n' && p[1] == 'a' && p[2] == 'n') {
        *any = 1;
        w[0] = w[1] = w[2] = 0;
        w[3] = 0x7FF8;
        return p + 3;
    }
    w[0] = w[1] = w[2] = w[3] = 0;
    for (i = 0; i < 16; i++) {
        int v = hexval(p[i]);
        if (v < 0)
            return 0;
        w[3 - i / 4] = (w[3 - i / 4] << 4) | v;
    }
    return p + 16;
}

#ifdef WANT_SPECIAL
static char *get_int(char *p, int *v)
{
    unsigned u = 0;
    int neg = 0, n = 0;

    p = skip(p);
    if (*p == '-') {
        neg = 1;
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        u = u * 10 + (*p - '0');
        p++;
        n++;
    }
    if (n == 0)
        return 0;
    *v = neg ? -(int)u : (int)u;
    return p;
}
#endif

/* 0, EDOM or ERANGE */
static char *get_errno(char *p, int *e)
{
    p = skip(p);
    if (p[0] == '0') {
        *e = 0;
        return p + 1;
    }
    if (p[0] == 'E' && p[1] == 'D') {
        *e = EDOM;
        return p + 4;
    }
    if (p[0] == 'E' && p[1] == 'R') {
        *e = ERANGE;
        return p + 6;
    }
    return 0;
}

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* ---- the checks of one call ---- */

static void put_errno(int e)
{
    if (e == 0)
        t_puts("0");
    else if (e == EDOM)
        t_puts("EDOM");
    else if (e == ERANGE)
        t_puts("ERANGE");
    else
        put_int(e);
}

/* the result of a call, compared; prints a line on failure */
static void check_result(const char *name, const word *a, const word *b, const char *what, const word *got, const word *want, int any, unsigned bound)
{
    unsigned ulps;

    if (g_exact)
        bound = 0;
    if (result_ok(got, want, any, bound, &ulps))
        return;
    begin_fail(name);
    if (nfail <= MAXFAIL) {
        t_puts(" ");
        put_hex(a);
        if (b) {
            t_puts(" ");
            put_hex(b);
        }
        t_puts(" ");
        t_puts(what);
        t_puts(" got ");
        put_hex(got);
        t_puts(" want ");
        if (any)
            t_puts("nan");
        else
            put_hex(want);
        t_puts(" ulps ");
        if (ulps == 65535u)
            t_puts(">=65535");
        else
            t_putu(ulps);
    }
    end_fail();
}

static void check_errno(const char *name, const word *a, const word *b, int got, int want, int preset)
{
    if (got == want)
        return;
    begin_fail(name);
    if (nfail <= MAXFAIL) {
        t_puts(" ");
        put_hex(a);
        if (b) {
            t_puts(" ");
            put_hex(b);
        }
        t_puts(preset ? " errno (set to " : " errno (cleared) ");
        if (preset) {
            put_int(SENTINEL);
            t_puts(") ");
        }
        t_puts("got ");
        put_errno(got);
        t_puts(" want ");
        put_errno(want);
    }
    end_fail();
}

#ifdef WANT_SPECIAL
static void check_int(const char *name, const word *a, const char *what, int got, int want)
{
    if (got == want)
        return;
    begin_fail(name);
    if (nfail <= MAXFAIL) {
        t_puts(" ");
        put_hex(a);
        t_puts(" ");
        t_puts(what);
        t_puts(" got ");
        put_int(got);
        t_puts(" want ");
        put_int(want);
    }
    end_fail();
}
#endif

/* ---- the functions with a double argument and a double result ---- */

#ifdef WANT_F1

typedef double (__cdecl *fn1_t)(double);

struct f1 {
    const char *name;
    fn1_t f;
    unsigned bound;             /* ulps */
};

static const struct f1 tab1[] = {
    { "sin", sin, 4 }, { "cos", cos, 4 }, { "tan", tan, 4 },
    { "asin", asin, 4 }, { "acos", acos, 4 }, { "atan", atan, 4 },
    { "sinh", sinh, 4 }, { "cosh", cosh, 4 }, { "tanh", tanh, 4 },
    { "exp", exp, 4 }, { "log", log, 4 }, { "log10", log10, 4 },
    { "sqrt", sqrt, 0 }, { "fabs", fabs, 0 }, { "floor", floor, 0 }, { "ceil", ceil, 0 },
};

static const struct f1 *find1(const char *name)
{
    unsigned i;

    for (i = 0; i < sizeof tab1 / sizeof tab1[0]; i++)
        if (streq(tab1[i].name, name))
            return &tab1[i];
    return 0;
}

static void row1(const struct f1 *t, char *p)
{
    dbl a, r;
    word want[4];
    int any, ewant, e, pass;

    if (!(p = get_bits(p, a.w, &any)) || !(p = get_bits(p, want, &any)) || !(p = get_errno(p, &ewant))) {
        fail_text(t->name, "bad line");
        return;
    }
    for (pass = 0; pass < (g_preset && ewant == 0 ? 2 : 1); pass++) {
        errno = pass ? SENTINEL : 0;
        r.d = t->f(a.d);
        e = errno;
        if (pass == 0) {
            check_result(t->name, a.w, 0, "result", r.w, want, any, t->bound);
            check_errno(t->name, a.w, 0, e, ewant, 0);
        } else {
            check_errno(t->name, a.w, 0, e, SENTINEL, 1);
        }
    }
}

#endif

/* ---- the functions of two doubles ---- */

#ifdef WANT_F2

typedef double (__cdecl *fn2_t)(double, double);

struct f2 {
    const char *name;
    fn2_t f;
    unsigned bound;
};

static const struct f2 tab2[] = {
    { "atan2", atan2, 4 }, { "pow", pow, 4 }, { "fmod", fmod, 0 },
};

static const struct f2 *find2(const char *name)
{
    unsigned i;

    for (i = 0; i < sizeof tab2 / sizeof tab2[0]; i++)
        if (streq(tab2[i].name, name))
            return &tab2[i];
    return 0;
}

static void row2(const struct f2 *t, char *p)
{
    dbl a, b, r;
    word want[4];
    int any, ewant, e, pass;

    if (!(p = get_bits(p, a.w, &any)) || !(p = get_bits(p, b.w, &any)) || !(p = get_bits(p, want, &any)) || !(p = get_errno(p, &ewant))) {
        fail_text(t->name, "bad line");
        return;
    }
    for (pass = 0; pass < (g_preset && ewant == 0 ? 2 : 1); pass++) {
        errno = pass ? SENTINEL : 0;
        r.d = t->f(a.d, b.d);
        e = errno;
        if (pass == 0) {
            check_result(t->name, a.w, b.w, "result", r.w, want, any, t->bound);
            check_errno(t->name, a.w, b.w, e, ewant, 0);
        } else {
            check_errno(t->name, a.w, b.w, e, SENTINEL, 1);
        }
    }
}

#endif

/* ---- frexp, modf and ldexp (all exact) ---- */

#ifdef WANT_SPECIAL

/* frexp: ARG MANTISSA EXPONENT ERRNO */
static void row_frexp(char *p)
{
    dbl a, r;
    word want[4];
    int any, ex, wantex, ewant, e, pass;

    if (!(p = get_bits(p, a.w, &any)) || !(p = get_bits(p, want, &any)) || !(p = get_int(p, &wantex)) || !(p = get_errno(p, &ewant))) {
        fail_text("frexp", "bad line");
        return;
    }
    for (pass = 0; pass < (g_preset && ewant == 0 ? 2 : 1); pass++) {
        errno = pass ? SENTINEL : 0;
        ex = SENTINEL;
        r.d = frexp(a.d, &ex);
        e = errno;
        if (pass == 0) {
            check_result("frexp", a.w, 0, "mantissa", r.w, want, any, 0);
            check_int("frexp", a.w, "exponent", ex, wantex);
            check_errno("frexp", a.w, 0, e, ewant, 0);
        } else {
            check_errno("frexp", a.w, 0, e, SENTINEL, 1);
        }
    }
}

/* modf: ARG FRACTION INTEGRAL ERRNO */
static void row_modf(char *p)
{
    dbl a, r, ip;
    word want[4], wanti[4];
    int any, anyi, ewant, e, pass;

    if (!(p = get_bits(p, a.w, &any)) || !(p = get_bits(p, want, &any)) || !(p = get_bits(p, wanti, &anyi)) || !(p = get_errno(p, &ewant))) {
        fail_text("modf", "bad line");
        return;
    }
    for (pass = 0; pass < (g_preset && ewant == 0 ? 2 : 1); pass++) {
        errno = pass ? SENTINEL : 0;
        ip.w[0] = ip.w[1] = ip.w[2] = ip.w[3] = 0x5A5A;
        r.d = modf(a.d, &ip.d);
        e = errno;
        if (pass == 0) {
            check_result("modf", a.w, 0, "fraction", r.w, want, any, 0);
            check_result("modf", a.w, 0, "integral", ip.w, wanti, anyi, 0);
            check_errno("modf", a.w, 0, e, ewant, 0);
        } else {
            check_errno("modf", a.w, 0, e, SENTINEL, 1);
        }
    }
}

/* ldexp: ARG EXPONENT RESULT ERRNO */
static void row_ldexp(char *p)
{
    dbl a, r;
    word want[4];
    int any, ex, ewant, e, pass;

    if (!(p = get_bits(p, a.w, &any)) || !(p = get_int(p, &ex)) || !(p = get_bits(p, want, &any)) || !(p = get_errno(p, &ewant))) {
        fail_text("ldexp", "bad line");
        return;
    }
    for (pass = 0; pass < (g_preset && ewant == 0 ? 2 : 1); pass++) {
        errno = pass ? SENTINEL : 0;
        r.d = ldexp(a.d, ex);
        e = errno;
        if (pass == 0) {
            check_result("ldexp", a.w, 0, "result", r.w, want, any, 0);
            check_errno("ldexp", a.w, 0, e, ewant, 0);
        } else {
            check_errno("ldexp", a.w, 0, e, SENTINEL, 1);
        }
    }
}

#endif

/* ---- one line, the file, the program ---- */

static void row(char *line)
{
    char name[16];
    char *p = get_name(line, name);

#ifdef WANT_F1
    {
        const struct f1 *t = find1(name);
        if (t) {
            row1(t, p);
            return;
        }
    }
#endif
#ifdef WANT_F2
    {
        const struct f2 *t = find2(name);
        if (t) {
            row2(t, p);
            return;
        }
    }
#endif
#ifdef WANT_SPECIAL
    if (streq(name, "frexp")) {
        row_frexp(p);
        return;
    }
    if (streq(name, "modf")) {
        row_modf(p);
        return;
    }
    if (streq(name, "ldexp")) {
        row_ldexp(p);
        return;
    }
#endif
    fail_text(name, "unknown function");
}

/* Runs the file named by the argument; the exit status is 1 if a line failed. */
static int driver(int argc, char **argv, int exact, int preset)
{
    char buf[128];
    FILE *fp;

    g_exact = exact;
    g_preset = preset;
    if (argc != 2) {
        t_puts("usage: PROGRAM FILE\r\n");
        return 2;
    }
    fp = fopen(argv[1], "r");
    if (!fp) {
        t_puts("cannot open ");
        t_puts(argv[1]);
        t_puts("\r\n");
        return 2;
    }
    while (fgets(buf, sizeof buf, fp)) {
        lineno++;
        if (is_end(skip(buf)) || skip(buf)[0] == '#')
            continue;
        nrows++;
        row(buf);
    }
    fclose(fp);
    if (nrows == 0) {
        t_puts("no rows in ");
        t_puts(argv[1]);
        t_puts("\r\n");
        return 2;
    }
    if (nfail) {
        t_putu(nfail);
        t_puts(" of ");
        t_putu(nrows);
        t_puts(" rows failed\r\n");
        return 1;
    }
    t_puts("ok\r\n");
    return 0;
}

#endif
