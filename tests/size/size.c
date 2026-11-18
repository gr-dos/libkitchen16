/* size.c - one program per profile, for `make size` (tests/size/report):
 * compiled with -DSZ_PROFILE, linked like every test program, and never run.
 * What a profile adds to the executable over SZ_base, which calls nothing
 * of libkitchen16's, is what a program that makes those calls pays for
 * them: libkitchen16's members and whatever of libc16 they bring in.
 *
 * Every call is on volatile operands, so the compiler keeps it; the
 * results go to volatile variables. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <kitchen16.h>
#include "testio.h"

/* global, so that a profile which uses none of them (base) still compiles */
volatile double a = 1.5, b = 2.5, r;
volatile float f;
volatile long l = 7;
volatile unsigned long u = 7;
volatile int i;
static char buf[32];

#if defined(SZ_perl) || defined(SZ_all)
static int vsp(char *s, const char *format, ...)
{
    va_list ap;
    int n;

    va_start(ap, format);
    n = vsprintf(s, format, ap);
    va_end(ap);
    return n;
}
#endif

int main(void)
{
#if defined(SZ_add)
    r = a + b;
#endif
#if defined(SZ_arith) || defined(SZ_perl) || defined(SZ_all)
    /* every fpc helper: what any -fpc program doing arithmetic may link */
    r = a + b; r = a - b; r = a * b; r = a / b; i = a < b; r = -a;
    l = (long)a; u = (unsigned long)a; r = (double)l; r = (double)u;
    f = (float)a; r = (double)f;
#endif
#if defined(SZ_snprintf)
    i = snprintf(buf, sizeof buf, "%g", a);
#endif
#if defined(SZ_sprintf) || defined(SZ_perl) || defined(SZ_all)
    i = sprintf(buf, "%.20g", a);
#endif
#if defined(SZ_printf)
    i = printf("%g\n", a);
#endif
#if defined(SZ_perl) || defined(SZ_all)
    i = fprintf(stdout, "%g\n", a);
    i = vsp(buf, "%d", 1);
#endif
#if defined(SZ_all)
    i = printf("%g\n", a);
    i = snprintf(buf, sizeof buf, "%g", a);
#endif
#if defined(SZ_strtod) || defined(SZ_all)
    r = strtod("1.5", (char **)0);
#endif
#if defined(SZ_perl) || defined(SZ_all)
    r = atof("1.5");
#endif
#if defined(SZ_sqrt) || defined(SZ_perl) || defined(SZ_all)
    r = sqrt(a);
#endif
#if defined(SZ_sin) || defined(SZ_perl) || defined(SZ_all)
    r = sin(a);
#endif
#if defined(SZ_perl) || defined(SZ_all)
    r = cos(a);
    {
        double ip;
        r = modf(a, &ip);
    }
#endif
#if defined(SZ_exp) || defined(SZ_perl) || defined(SZ_all)
    r = exp(a);
#endif
#if defined(SZ_log) || defined(SZ_perl) || defined(SZ_all)
    r = log(a);
#endif
#if defined(SZ_pow) || defined(SZ_perl) || defined(SZ_all)
    r = pow(a, b);
#endif
#if defined(SZ_atan2) || defined(SZ_perl) || defined(SZ_all)
    r = atan2(a, b);
#endif
#if defined(SZ_all)
    {
        int e;
        r = tan(a); r = asin(a); r = acos(a); r = atan(a);
        r = sinh(a); r = cosh(a); r = tanh(a); r = log10(a);
        r = floor(a); r = ceil(a); r = fabs(a); r = fmod(a, b);
        r = frexp(a, &e); r = ldexp(a, 3);
    }
#endif
    t_puts(buf);
    return 0;
}
