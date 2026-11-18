/* k.h - what the members of <math.h> share (private to src/): the view of a
 * double as four words, and the internal functions that more than one
 * public function needs. Internal functions use the default register
 * convention; a double argument or result is AX:BX:CX:DX. */

#ifndef K_H
#define K_H

#include <math.h>

typedef unsigned short word;

typedef union {
    double d;
    word w[4];                  /* w[3] has the sign and the exponent */
} kdbl;

#define K_BE(u)   (((u).w[3] >> 4) & 0x7FF)             /* biased exponent field */
#define K_SIGN(u) ((u).w[3] >> 15)
#define K_FRAC0(u) (((u).w[3] & 0x000F) | (u).w[2] | (u).w[1] | (u).w[0])
#define K_ISNAN(u) (K_BE(u) == 0x7FF && K_FRAC0(u) != 0)
#define K_ISINF(u) (K_BE(u) == 0x7FF && K_FRAC0(u) == 0)
#define K_ISZERO(u) (((u).w[3] & 0x7FFF) == 0 && ((u).w[2] | (u).w[1] | (u).w[0]) == 0)
#define K_SETINF(u, neg) ((u).w[0] = (u).w[1] = (u).w[2] = 0, (u).w[3] = (neg) ? 0xFFF0 : 0x7FF0)
#define K_SETNAN(u)      ((u).w[0] = (u).w[1] = (u).w[2] = 0, (u).w[3] = 0x7FF8)
#define K_SETZERO(u, neg) ((u).w[0] = (u).w[1] = (u).w[2] = 0, (u).w[3] = (neg) ? 0x8000 : 0)
#define K_QUIET(u)       ((u).w[3] |= 0x0008)

/* e^a / 2^n for a = ah + al, |a| <= 745, as a pair y[0] + y[1] (y[0] about
 * 1, within a few ulps; y[1] its rounding error and the correction); returns
 * n. */
int kexp(double ah, double al, double *y);

/* ln x for finite x > 0 (subnormals too), as r[0] + r[1] with a relative
 * error of about 2^-62. */
void klog(double x, double *r);

/* sin (mode 0), cos (1) or tan (2) of finite x. */
double ktrig(double x, int mode);

/* atan t for t = th + tl >= 0 (th may be +inf). */
double katan2(double th, double tl);

/* a / sqrt(1 - a^2) for a = ah + al in [0, 1), as t[0] + t[1]. */
void kasq(double ah, double al, double *t);

/* sqrt x, correctly rounded; a NaN for x < 0, x itself for -0. */
double ksqrt(double x);

/* x * 2^n; the result in the subnormal range is rounded to nearest even;
 * an overflow gives an infinity and an underflow a zero. */
double kscale(double x, int n);

/* The cases of pow that need no logarithm: returns 0 with the result in
 * *out, or 1 (positive) or 2 (negative) with |x| in *out when x^y is
 * exp(y ln|x|) with x finite and non-zero, y finite and not an integer
 * of at most 1024 in magnitude. */
int kpowc(double x, double y, double *out);

/* x^y for |x| > 0 and an integer |y| <= 1024 (y > 0 or y < 0): the result
 * with the sign of x and y odd; 0 and +inf when it underflows or overflows. */
double kpowi(double x, int y);

/* the error terms (k_dd.c): the rounded sum or product, and the exact error
 * through *e */
double kd_sum(double a, double b, double *e);
double kd_prod(double a, double b, double *e);
double kd_prodc(double a, double b, double bh, double bl, double *e);

#endif
