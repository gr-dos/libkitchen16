/* math.h - libkitchen16 (C89 7.5; docs/spec/math.md)
 *
 * Every function is __cdecl and takes and returns double. Include libc16's
 * headers, then this one. Compile with -fpc (docs/ABI.md). */

#ifndef _LIBKITCHEN16_MATH_H
#define _LIBKITCHEN16_MATH_H

#include <_libc16.h>

/* +inf. An object, so it is not usable in a static initialiser. */
extern const double __cdecl __kitchen16_huge_val;
#define HUGE_VAL (__kitchen16_huge_val)

double __cdecl acos(double x);
double __cdecl asin(double x);
double __cdecl atan(double x);
double __cdecl atan2(double y, double x);
double __cdecl ceil(double x);
double __cdecl cos(double x);
double __cdecl cosh(double x);
double __cdecl exp(double x);
double __cdecl fabs(double x);
double __cdecl floor(double x);
double __cdecl fmod(double x, double y);
double __cdecl frexp(double x, int *exp);
double __cdecl ldexp(double x, int exp);
double __cdecl log(double x);
double __cdecl log10(double x);
double __cdecl modf(double x, double *iptr);
double __cdecl pow(double x, double y);
double __cdecl sin(double x);
double __cdecl sinh(double x);
double __cdecl sqrt(double x);
double __cdecl tan(double x);
double __cdecl tanh(double x);

#endif
