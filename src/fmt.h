/* fmt.h - the conversion engine shared by the members of fmt and fmtio
 * (docs/spec/fmt.md). Private to the library. */

#ifndef _LIBKITCHEN16_FMT_H
#define _LIBKITCHEN16_FMT_H

#include <_libc16.h>
#include <stddef.h>
#include <stdarg.h>

/* Called with every output character in order; returns nonzero to stop. */
typedef int (*__k16_out)(void *ctx, int c);

/* Formats into out. limit nonzero: stop at the 32768th character (the
 * memory functions); zero: carry on, counting no further (the stream
 * functions). Returns the number of characters, or -1 with errno ERANGE if
 * there were more than 32767, or -1 (errno untouched) if out asked to stop
 * before that. */
int __cdecl __kitchen16_fmt(__k16_out out, void *ctx, const char *format,
                            va_list ap, int limit);

#endif
