/* vfprintf (docs/spec/fmtio.md): the engine's output goes to fwrite in
 * blocks of 128 characters. */

#include <kitchen16.h>
#include "fmt.h"

#define BLOCK 128

struct strsink {
    FILE *fp;
    unsigned n;
    char buf[BLOCK];
};

static int flush(struct strsink *s)
{
    unsigned n = s->n;

    s->n = 0;
    return n != 0 && fwrite(s->buf, 1, n, s->fp) != n;
}

static int str_out(void *ctx, int c)
{
    struct strsink *s = ctx;

    s->buf[s->n++] = (char)c;
    return s->n == BLOCK && flush(s);
}

int __cdecl vfprintf(FILE *fp, const char *format, va_list ap)
{
    struct strsink s;
    int r;

    if (fp == 0)
        return -1;
    s.fp = fp;
    s.n = 0;
    r = __kitchen16_fmt(str_out, &s, format, ap, 0);
    /* a failed block is not retried: its characters are gone (n = 0) and
     * the engine has stopped, so a negative result with nothing pending
     * means an error, and with ERANGE the characters are still to write */
    if (flush(&s))
        return -1;
    return r;
}
