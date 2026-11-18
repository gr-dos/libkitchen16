/* The 32767-character limit, widths and precisions beyond it, and that
 * nothing is written outside the target (docs/spec/fmt.md, "Behaviour"
 * and "The format"): a buffer of exactly 32768 bytes between guard bytes. */

#include <errno.h>
#include <string.h>
#include <kitchen16.h>
#include "check.h"

#define SIZE  32768u
#define GUARD 16u
#define PAT   0xA5

static unsigned char mem[GUARD + SIZE + GUARD];
#define T ((char *)mem + GUARD)

static void clear(void)
{
    memset(mem, PAT, sizeof mem);
}

/* the guards are intact, and bytes from `from` on in the buffer too */
static int intact(unsigned from)
{
    unsigned i;

    for (i = 0; i < GUARD; i++)
        if (mem[i] != PAT || mem[GUARD + SIZE + i] != PAT)
            return 0;
    for (i = from; i < SIZE; i++)
        if (T[i] != (char)PAT)
            return 0;
    return 1;
}

/* T[0 .. n-1] is `c` throughout, except `at`, which is `other` */
static int run_of(unsigned n, char c, unsigned at, char other)
{
    unsigned i;

    for (i = 0; i < n; i++)
        if (T[i] != (i == at ? other : c))
            return 0;
    return 1;
}

/* T[from .. to-1] are all zeros */
static int zeros(unsigned from, unsigned to)
{
    for (; from < to; from++)
        if (T[from] != '0')
            return 0;
    return 1;
}

static int wrap_vsprintf(char *s, const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vsprintf(s, f, ap);
}

static int wrap_vsnprintf(char *s, size_t n, const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vsnprintf(s, n, f, ap);
}

int main(void)
{
    int r;

    /* exactly 32767 characters: fine, 32768 bytes with the null */
    clear();
    errno = 0;
    r = sprintf(T, "%.32767d", 1);
    CHECK(r == 32767 && errno == 0);
    CHECK(run_of(32767, '0', 32766, '1') && T[32767] == 0 && intact(32768));

    clear();
    r = wrap_vsprintf(T, "%.32766d%c", 1, 'x');
    CHECK(r == 32767 && run_of(32766, '0', 32765, '1') && T[32766] == 'x' && T[32767] == 0 && intact(32768));

    /* 32768: -1, ERANGE, 32767 characters and the null stay */
    clear();
    errno = 0;
    r = sprintf(T, "%.32767dx", 1);
    CHECK(r == -1 && errno == ERANGE);
    CHECK(run_of(32767, '0', 32766, '1') && T[32767] == 0 && intact(32768));

    clear();
    errno = 0;
    r = wrap_vsprintf(T, "%.20000d%.20000d", 1, 2);
    CHECK(r == -1 && errno == ERANGE);
    CHECK(run_of(32767, '0', 19999, '1') && T[32767] == 0 && intact(32768));

    /* snprintf, vsnprintf: the limit and n together */
    clear();
    errno = 0;
    r = snprintf(T, SIZE, "%.20000d%.20000d", 1, 2);
    CHECK(r == -1 && errno == ERANGE);
    CHECK(run_of(32767, '0', 19999, '1') && T[32767] == 0 && intact(32768));

    clear();
    errno = 0;
    r = wrap_vsnprintf(T, 100, "%.20000d%.20000d", 1, 2);
    CHECK(r == -1 && errno == ERANGE);
    CHECK(run_of(99, '0', 99, '1') && T[99] == 0 && intact(100));

    clear();
    errno = 0;
    r = snprintf(T, 1, "%.32767dx", 1);
    CHECK(r == -1 && errno == ERANGE && T[0] == 0 && intact(1));

    clear();
    errno = 0;
    r = snprintf((char *)0, 0, "%.20000d%.20000d", 1, 2);
    CHECK(r == -1 && errno == ERANGE);
    CHECK(intact(0));

    /* n of 0 with a null pointer: the length, nothing written */
    r = snprintf((char *)0, 0, "abc%d", 12);
    CHECK(r == 5);
    r = wrap_vsnprintf((char *)0, 0, "%s|%5d", "xy", 3);
    CHECK(r == 8);

    /* a short n under the limit: the length it would have had */
    clear();
    r = snprintf(T, 10, "%.20000d", 1);
    CHECK(r == 20000 && run_of(9, '0', 9, '1') && T[9] == 0 && intact(10));

    /* widths and precisions beyond 32767 are 32767 */
    clear();
    r = sprintf(T, "%99999d", 1);
    CHECK(r == 32767 && run_of(32766, ' ', 65535u, ' ') && T[32766] == '1' && T[32767] == 0 && intact(32768));

    clear();
    r = sprintf(T, "%-40000d", 1);
    CHECK(r == 32767 && T[0] == '1' && run_of(32767, ' ', 0, '1') && T[32767] == 0 && intact(32768));

    clear();
    r = sprintf(T, "%*d", -32767 - 1, 5);    /* the absolute value of INT_MIN is taken as 32767 */
    CHECK(r == 32767 && T[0] == '5' && T[1] == ' ' && T[32766] == ' ' && T[32767] == 0 && intact(32768));

    clear();
    r = sprintf(T, "%.*d", 32767, 5);
    CHECK(r == 32767 && run_of(32767, '0', 32766, '5') && intact(32768));

    clear();
    r = sprintf(T, "%.*d", -32767 - 1, 5);    /* negative: no precision */
    CHECK(r == 1 && T[0] == '5' && T[1] == 0 && intact(2));

    clear();
    r = sprintf(T, "%.*d|", -1, 5);
    CHECK(r == 2 && T[0] == '5' && T[1] == '|' && T[2] == 0 && intact(3));

    clear();
    r = sprintf(T, "%.99999s", "short");
    CHECK(r == 5 && T[4] == 't' && T[5] == 0 && intact(6));

    /* floating conversions at every precision up to the limit */
    clear();
    {
        union { unsigned short w[4]; double d; } one;

        one.w[3] = 0x3FF0;
        one.w[2] = one.w[1] = one.w[0] = 0;
        r = sprintf(T, "%.32765f", one.d);    /* "1." and 32765 zeros */
        CHECK(r == 32767 && T[0] == '1' && T[1] == '.' && zeros(2, 32767));
        CHECK(T[32767] == 0 && intact(32768));

        clear();
        errno = 0;
        r = sprintf(T, "%.32766f", one.d);
        CHECK(r == -1 && errno == ERANGE && T[0] == '1' && T[1] == '.' && zeros(2, 32767) && T[32767] == 0 && intact(32768));

        clear();
        errno = 0;
        r = snprintf(T, 20, "%.32767e", one.d);    /* 1.000...e+00 would be 32773 characters */
        CHECK(r == -1 && errno == ERANGE && T[0] == '1' && T[1] == '.' && zeros(2, 19) && T[19] == 0 && intact(20));

        clear();
        r = sprintf(T, "%.32767g", one.d);    /* trailing zeros removed */
        CHECK(r == 1 && T[0] == '1' && T[1] == 0 && intact(2));

        clear();
        r = sprintf(T, "%#.32760g", one.d);    /* kept: "1." and 32759 zeros */
        CHECK(r == 32761 && T[0] == '1' && T[1] == '.' && zeros(2, 32761) && T[32761] == 0 && intact(32762));

    }

    /* the limit counts a sum of conversions: nothing is lost past it */
    clear();
    errno = 0;
    r = sprintf(T, "%.16384d%.16384d", 1, 2);
    CHECK(r == -1 && errno == ERANGE && T[32767] == 0 && intact(32768));

    return done();
}
