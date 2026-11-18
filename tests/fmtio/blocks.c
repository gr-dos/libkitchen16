/* The output is handed to fwrite in blocks (docs/spec/fmtio.md,
 * "Behaviour"): this program defines its own fwrite, which counts its
 * calls and keeps what it is given, and which links ahead of libc16's.
 * 1,000 characters make at most 8 calls, none of them with more than 128
 * characters, and after a failed call (fewer characters written than
 * asked) the function stops, and returns -1. The stream is stdout, which
 * the hook never touches. */

#include <string.h>
#include <kitchen16.h>
#include "check.h"

#define KEEP 1100

static unsigned calls, biggest, total, overflow;
static unsigned fail_after;    /* 0: never; n: the nth call and later ones fail */
static unsigned short_by;      /* the nth call writes this many characters fewer than asked (0: none) */
static char kept[KEEP];

size_t __cdecl fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    unsigned bytes = (unsigned)(size * nmemb);

    (void)fp;
    calls++;
    if (fail_after && calls >= fail_after) {
        /* this call and every later one fail: the test counts them */
        unsigned put = 0;

        if (calls == fail_after && short_by && short_by < bytes)
            put = bytes - short_by;
        if (total + put <= KEEP)
            memcpy(kept + total, ptr, put);
        total += put;
        return put / size;
    }
    if (bytes > biggest)
        biggest = bytes;
    if (total + bytes <= KEEP)
        memcpy(kept + total, ptr, bytes);
    else
        overflow = 1;
    total += bytes;
    return nmemb;
}

static void reset(unsigned after, unsigned by)
{
    calls = biggest = total = overflow = 0;
    fail_after = after;
    short_by = by;
    memset(kept, '?', sizeof kept);
}

static char fmt1000[1001];
static char str1000[1001];
static char expect[1100];

static int wrap_vfprintf(FILE *fp, const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vfprintf(fp, f, ap);
}

static int wrap_vprintf(const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vprintf(f, ap);
}

/* what the hook was given is `n` characters equal to expect[0 .. n-1],
 * in at most 8 calls (for n of 1000), none over 128 characters */
static void blocks_ok(int ret, unsigned n, int line)
{
    if (ret != (int)n || total != n || overflow || memcmp(kept, expect, n) != 0 || biggest > 128 || calls == 0
        || (n >= 1000 && calls > 8)) {
        t_puts("not ok ");
        t_putu((unsigned)line);
        t_puts(": ret ");
        t_putu((unsigned)ret);
        t_puts(" total ");
        t_putu(total);
        t_puts(" calls ");
        t_putu(calls);
        t_puts(" biggest ");
        t_putu(biggest);
        t_puts("\r\n");
        failures++;
    }
}

#define BLOCKS(call, n) \
    do { \
        reset(0, 0); \
        blocks_ok(call, n, __LINE__); \
    } while (0)

int main(void)
{
    unsigned i;
    int r;
    union { unsigned short w[4]; double d; } z;

    for (i = 0; i < 1000; i++) {
        fmt1000[i] = (char)('a' + i % 26);
        str1000[i] = (char)('A' + i % 26);
    }

    /* 1,000 characters of literal text */
    memcpy(expect, fmt1000, 1000);
    BLOCKS(fprintf(stdout, fmt1000), 1000);
    BLOCKS(wrap_vfprintf(stdout, fmt1000), 1000);
    BLOCKS(printf(fmt1000), 1000);
    BLOCKS(wrap_vprintf(fmt1000), 1000);

    /* a string argument of 1,000 characters */
    memcpy(expect, str1000, 1000);
    BLOCKS(fprintf(stdout, "%s", str1000), 1000);
    BLOCKS(printf("%.1000s", str1000), 1000);

    /* padding: spaces and zeros that the conversion makes */
    memset(expect, ' ', 999);
    expect[999] = '7';
    BLOCKS(fprintf(stdout, "%1000d", 7), 1000);
    BLOCKS(printf("%*d", 1000, 7), 1000);
    memset(expect, ' ', 1000);
    expect[0] = '7';
    BLOCKS(fprintf(stdout, "%-1000d", 7), 1000);
    memset(expect, '0', 1000);
    expect[999] = '7';
    BLOCKS(fprintf(stdout, "%.1000d", 7), 1000);
    BLOCKS(wrap_vfprintf(stdout, "%01000d", 7), 1000);

    /* a floating conversion with a long fraction: "0." and 998 zeros */
    z.w[3] = z.w[2] = z.w[1] = z.w[0] = 0;
    memset(expect, '0', 1000);
    expect[1] = '.';
    BLOCKS(fprintf(stdout, "%.998f", z.d), 1000);

    /* many small pieces: 250 copies of "ab%dc" with 7 = "ab7c" */
    {
        static char many[250 * 5 + 1];

        for (i = 0; i < 250; i++)
            memcpy(many + 5 * i, "ab%dc", 5);
        many[1250] = 0;
        for (i = 0; i < 250; i++)
            memcpy(expect + 4 * i, "ab7c", 4);
        /* 250 conversions need 250 arguments: use a vfprintf with a word array */
        {
            static unsigned words[250];

            for (i = 0; i < 250; i++)
                words[i] = 7;
            reset(0, 0);
            r = vfprintf(stdout, many, (va_list)words);
            blocks_ok(r, 1000, __LINE__);
        }
    }

    /* a write that fails stops the output: 1,000 characters, the third call short or failing */
    memcpy(expect, fmt1000, 1000);
    reset(3, 0);
    r = fprintf(stdout, fmt1000);
    CHECK(r < 0);
    CHECK(calls == 3);    /* nothing is written after the failure */
    reset(3, 5);
    r = wrap_vfprintf(stdout, "%s", str1000);
    CHECK(r < 0);
    CHECK(calls == 3);
    reset(1, 0);
    r = printf("%.500d", 5);
    CHECK(r < 0);
    CHECK(calls == 1);

    /* the very first block failing */
    reset(1, 1);
    r = fprintf(stdout, "short %d", 5);
    CHECK(r < 0);
    CHECK(calls == 1);

    return done();
}
