/* snprintf (docs/spec/fmt.md): the engine directly, so that a program that
 * calls it does not link vsnprintf. */

#include <kitchen16.h>
#include "fmt.h"

struct memsink {
    char *p;
    unsigned room;      /* characters that may still be stored, plus one */
};

static int mem_out(void *ctx, int c)
{
    struct memsink *m = ctx;

    if (m->room > 1) {
        *m->p++ = (char)c;
        m->room--;
    }
    return 0;
}

int __cdecl snprintf(char *s, size_t n, const char *format, ...)
{
    struct memsink m;
    va_list ap;
    int r;

    va_start(ap, format);
    m.p = s;
    m.room = n;
    r = __kitchen16_fmt(mem_out, &m, format, ap, 1);
    if (n != 0)
        *m.p = 0;
    return r;
}
