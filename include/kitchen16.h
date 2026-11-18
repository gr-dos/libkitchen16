/* kitchen16.h - libkitchen16: the formatted output family and string to
 * double conversion that libc16 leaves out (docs/spec/fmt.md, fmtio.md and
 * conv.md).
 *
 * Include libc16's headers first, then this one: it uses libc16's size_t,
 * va_list and FILE and adds to <stdio.h> and <stdlib.h> without changing
 * them. Every function is __cdecl, as libc16's are. */

#ifndef _LIBKITCHEN16_KITCHEN16_H
#define _LIBKITCHEN16_KITCHEN16_H

#include <_libc16.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

/* <stdio.h>: C89 7.9.6, C99 7.19.6 for the bounded forms. %e %f %g are
 * correctly rounded at any precision. Not supported: %n %p %a, C99 length
 * modifiers and wide characters. */
int __cdecl vsnprintf(char *s, size_t n, const char *format, va_list ap);
int __cdecl snprintf(char *s, size_t n, const char *format, ...);
int __cdecl vsprintf(char *s, const char *format, va_list ap);
int __cdecl sprintf(char *s, const char *format, ...);
int __cdecl vfprintf(FILE *fp, const char *format, va_list ap);
int __cdecl fprintf(FILE *fp, const char *format, ...);
int __cdecl vprintf(const char *format, va_list ap);
int __cdecl printf(const char *format, ...);

/* <stdlib.h>: correctly rounded; `inf`, `infinity` and `nan` are accepted,
 * hexadecimal floating constants are not. */
double __cdecl strtod(const char *nptr, char **endptr);
double __cdecl atof(const char *nptr);

#endif
