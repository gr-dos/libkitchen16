/* vprintf (docs/spec/fmtio.md). */

#include <kitchen16.h>

int __cdecl vprintf(const char *format, va_list ap)
{
    return vfprintf(stdout, format, ap);
}
