/* vsprintf (docs/spec/fmt.md). */

#include <kitchen16.h>
#include "fmt.h"

struct memsink {
    char *p;
    unsigned room;
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

int __cdecl vsprintf(char *s, const char *format, va_list ap)
{
    struct memsink m;
    int r;

    m.p = s;
    m.room = 32768u;
    r = __kitchen16_fmt(mem_out, &m, format, ap, 1);
    *m.p = 0;
    return r;
}
