/* The error terms of a sum and of a product (Knuth, Dekker): exact in
 * binary64 arithmetic with rounding to nearest. Functions rather than
 * macros: a call keeps its temporaries out of the caller's stack frame, and
 * the statements are simple ones, because the compiler gives each nested
 * sub-expression a stack slot of its own. */

#include "k.h"

/* a + b rounded; *e the exact error */
double kd_sum(double a, double b, double *e)
{
    double s, t, u;

    s = a + b;
    t = s - a;
    u = s - t;
    a = a - u;
    b = b - t;
    *e = a + b;
    return s;
}

/* a * b rounded; *e the exact error (|a|, |b| below 2^995) */
double kd_prod(double a, double b, double *e)
{
    double p, ah, al, bh, bl, t;

    p = a * b;
    ah = a * 134217729.0;               /* 2^27 + 1: a = ah + al, ah its top 26 bits */
    al = ah - a;
    ah = ah - al;
    al = a - ah;
    bh = b * 134217729.0;
    bl = bh - b;
    bh = bh - bl;
    bl = b - bh;
    t = ah * bh;
    t = t - p;
    ah = ah * bl;
    t = t + ah;
    bh = al * bh;
    t = t + bh;
    al = al * bl;
    t = t + al;
    *e = t;
    return p;
}

/* the same for a constant b given with its split bh + bl (a compiler may
 * fold the split of a constant into something else) */
double kd_prodc(double a, double b, double bh, double bl, double *e)
{
    double p, ah, al, t;

    p = a * b;
    ah = a * 134217729.0;
    al = ah - a;
    ah = ah - al;
    al = a - ah;
    t = ah * bh;
    t = t - p;
    ah = ah * bl;
    t = t + ah;
    bh = al * bh;
    t = t + bh;
    al = al * bl;
    t = t + al;
    *e = t;
    return p;
}
