/* fprintf (docs/spec/fmtio.md). */

#include <kitchen16.h>

int __cdecl fprintf(FILE *fp, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    return vfprintf(fp, format, ap);
}
