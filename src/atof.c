/* atof: strtod without an end pointer (docs/spec/conv.md). */

#include <kitchen16.h>

double __cdecl atof(const char *nptr)
{
    return strtod(nptr, (char **)0);
}
