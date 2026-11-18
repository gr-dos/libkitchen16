/* stack.h - measure the stack a call uses (tests/stack; included by its
 * programs, everything static).
 *
 * measure() fills the stack below its own frame with a pattern, makes one
 * call with interrupts off, and finds the lowest byte the call changed. The
 * result is the bytes used below the stack pointer at the call: the call's
 * own arguments and return address, and everything the callee and the
 * functions it calls (the floating-point helpers included) used. The
 * programs link the real library, so these are the figures a consumer's
 * stack must allow. Each program has `option stack=STACKSIZE` (rules.mk).
 *
 * A call that does DOS I/O (fwrite to a file) runs with interrupts on once
 * DOS has turned them on, so a timer interrupt can add some bytes to the
 * figure: it is an upper bound plus a few tens of bytes there, and
 * exact for every call that does no I/O. */

#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kitchen16.h>
#include "check.h"

extern char __cdecl __libc16_stack_top[];

#define STACKSIZE 6144u
#define PATTERN   0xA5

static unsigned get_sp(void);
#pragma aux get_sp = "mov ax, sp" value [ax];
static void cli(void);
#pragma aux cli = "cli";
static void sti(void);
#pragma aux sti = "sti";

typedef void (__cdecl *fn_t)(void);

/* what measure() calls */
enum kind {
    K_D_D,      /* double f(double a) */
    K_D_DD,     /* double f(double a, double b) */
    K_D_DI,     /* double f(double a, int n) */
    K_D_DPI,    /* double f(double a, int *p) */
    K_D_DPD,    /* double f(double a, double *p) */
    K_STR,      /* double f(const char *p, char **end) */
    K_FPRINTF,  /* int f(FILE *p, const char *fmt, double a), the format in q */
    K_SNPRINTF, /* int f(char *p, size_t n, const char *fmt, double a), the format in q */
    K_VOID      /* void f(void), a default-convention function */
};

static volatile double sink_d;
static volatile int sink_i;

/* Bytes of stack used by the call, or 0xFFFF if it reached the bottom of the stack. */
static unsigned measure(enum kind kind, fn_t f, double a, double b, int n, void *p, const char *q)
{
    unsigned sp = get_sp();
    unsigned top = (unsigned)__libc16_stack_top;
    unsigned char *base = (unsigned char *)__libc16_stack_top;
    unsigned char *lo = base - STACKSIZE;
    unsigned char *at = base - (top - sp);
    unsigned char *s;
    int ri = 0;
    double rd = 0;
    int i;

    for (s = lo; s < at; s++)
        *s = PATTERN;
    cli();
    switch (kind) {
    case K_D_D:     rd = ((double (__cdecl *)(double))f)(a); break;
    case K_D_DD:    rd = ((double (__cdecl *)(double, double))f)(a, b); break;
    case K_D_DI:    rd = ((double (__cdecl *)(double, int))f)(a, n); break;
    case K_D_DPI:   rd = ((double (__cdecl *)(double, int *))f)(a, (int *)p); break;
    case K_D_DPD:   rd = ((double (__cdecl *)(double, double *))f)(a, (double *)p); break;
    case K_STR:     rd = ((double (__cdecl *)(const char *, char **))f)((const char *)p, (char **)0); break;
    case K_FPRINTF: ri = ((int (__cdecl *)(FILE *, const char *, ...))f)((FILE *)p, q, a); break;
    case K_SNPRINTF: ri = ((int (__cdecl *)(char *, size_t, const char *, ...))f)((char *)p, (size_t)n, q, a); break;
    case K_VOID:    ((void (*)(void))f)(); break;
    }
    sti();
    sink_d = rd;
    sink_i = ri;
    for (s = lo; s < at && *s == PATTERN; s++)
        ;
    i = (int)(s - lo);
    if (i < 16)
        return 0xFFFFu;    /* the bottom of the stack was reached */
    return (unsigned)(at - s);
}

/* The most used by any call of one function, and the call that did it. */
struct high { const char *name; unsigned used; unsigned at; };

static void note(struct high *h, unsigned used, unsigned index)
{
    if (used > h->used) {
        h->used = used;
        h->at = index;
    }
}

/* Prints the table when the program is given -v, and counts a failure for each
 * function over limit (0: no limit). */
static void report(struct high *h, unsigned count, unsigned limit, int verbose)
{
    unsigned i, most = 0;

    for (i = 0; i < count; i++) {
        if (h[i].used > most)
            most = h[i].used;
        if (verbose) {
            t_puts(h[i].name);
            t_puts(" ");
            t_putu(h[i].used);
            t_puts(" (case ");
            t_putu(h[i].at);
            t_puts(")\r\n");
        }
        CHECK(h[i].used != 0xFFFFu);
        CHECK(limit == 0 || h[i].used <= limit);
    }
    if (verbose) {
        t_puts("deepest ");
        t_putu(most);
        t_puts(" bytes of ");
        t_putu(STACKSIZE);
        t_puts("\r\n");
    }
}

#ifdef WANT_VALUES

/* A double from its four 16-bit words, high word first. */
static double dbl(unsigned h, unsigned m1, unsigned m2, unsigned l)
{
    union { double d; unsigned short w[4]; } u;

    u.w[3] = h; u.w[2] = m1; u.w[1] = m2; u.w[0] = l;
    return u.d;
}

/* The values the programs try: zeros, small and large, subnormal, the
 * limits of the format, values that need the hard paths (a huge argument
 * for the trigonometric reduction, the edge of overflow for exp), infinities
 * and a NaN. */
#define NV 29
static double V[NV];

static void values(void)
{
    int i = 0;

    V[i++] = 0.0;
    V[i++] = dbl(0x8000, 0, 0, 0);               /* -0 */
    V[i++] = 1.0;
    V[i++] = -1.0;
    V[i++] = 0.5;
    V[i++] = 2.0;
    V[i++] = 3.0;
    V[i++] = 10.0;
    V[i++] = 0.1;
    V[i++] = 20.0;
    V[i++] = 1.0005;
    V[i++] = 1e-300;
    V[i++] = 1e300;
    V[i++] = 1e22;                               /* the reduction of sin needs its widest path */
    V[i++] = 1e9;
    V[i++] = 709.78;                             /* the edge of overflow for exp */
    V[i++] = 710.0;
    V[i++] = -745.13;                            /* the edge of underflow */
    V[i++] = -746.0;
    V[i++] = dbl(0x7FEF, 0xFFFF, 0xFFFF, 0xFFFF);   /* DBL_MAX */
    V[i++] = dbl(0x0010, 0, 0, 0);               /* the smallest normal */
    V[i++] = dbl(0x0000, 0, 0, 1);               /* the smallest subnormal */
    V[i++] = dbl(0x000F, 0xFFFF, 0xFFFF, 0xFFFF);   /* the largest subnormal */
    V[i++] = dbl(0x3FF0, 0, 0, 1);               /* 1 + 2^-52 */
    V[i++] = dbl(0x3FEF, 0xFFFF, 0xFFFF, 0xFFFF);   /* 1 - 2^-53 */
    V[i++] = dbl(0xFFEF, 0xFFFF, 0xFFFF, 0xFFFF);   /* -DBL_MAX */
    V[i++] = dbl(0x7FF0, 0, 0, 0);               /* +inf */
    V[i++] = dbl(0xFFF0, 0, 0, 0);               /* -inf */
    V[i++] = dbl(0x7FF8, 0, 0, 0);               /* a NaN */
}

#endif /* WANT_VALUES */

static int has_v(int argc, char **argv)
{
    return argc > 1 && argv[1][0] == '-' && argv[1][1] == 'v';
}

#endif
