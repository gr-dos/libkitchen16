/* printf (docs/spec/fmtio.md). */

#include <kitchen16.h>

int __cdecl printf(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    return vfprintf(stdout, format, ap);
}
