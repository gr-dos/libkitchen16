/* driver.h - the corpus reader and the call helpers of tests/fmt and
 * tests/fmtio (included by their programs; everything is static).
 *
 * A data file (tests/fmt/gen) has one record per line, five tab-separated
 * fields:
 *
 *   MODE  FORMAT  ARGS  EXPECTED  RETURN
 *
 * MODE is P (output into a roomy buffer) or N and a number (snprintf with
 * that n). FORMAT and EXPECTED are written with \\, \t, \n, \r and \xHH
 * escapes. ARGS are space-separated tokens, i:N (int), l:N (long), d:HEX16
 * (a double, 16 hex digits, sign bit first), s:TEXT (a string, escaped as
 * the other fields, and spaces as \x20) and S: (a null pointer).
 *
 * The arguments are turned into 16-bit words, as a __cdecl call pushes them
 * (docs/ABI.md): an int is a word, a long two (low word first), a double
 * four (low word first), a pointer one in the small model and two (offset
 * and segment) otherwise. A variadic function is called with that many
 * unsigned words, a v function with a va_list that points at them: the
 * same words in the same places, so the corpus tests the calling
 * convention too, with no floating-point operation anywhere. */

#ifndef DRIVER_H
#define DRIVER_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <kitchen16.h>
#include "check.h"

#define LINEMAX 1700
#define MAXW    14
#define MAXSHOW 12

struct rec {
    char mode;              /* 'P' or 'N' */
    unsigned n;             /* for N */
    char *fmt;
    unsigned nw;
    unsigned w[MAXW];
    char *want;             /* the expected output (may hold null characters) */
    unsigned wantlen;
    int ret;
};

static char line[LINEMAX];
static unsigned lineno;
static unsigned shown;

/* ---- output of failures */

static void put_int(int v)
{
    if (v < 0) {
        t_puts("-");
        t_putu((unsigned)(-(v + 1)) + 1u);
    } else {
        t_putu((unsigned)v);
    }
}

/* s[0..n-1], escaped, at most 100 characters */
static void put_esc(const char *s, unsigned n)
{
    static const char hex[] = "0123456789ABCDEF";
    char out[5];
    unsigned i;

    t_puts("\"");
    for (i = 0; i < n && i < 100; i++) {
        unsigned char c = (unsigned char)s[i];

        if (c >= 0x20 && c < 0x7F && c != '\\') {
            out[0] = (char)c;
            out[1] = 0;
        } else {
            out[0] = '\\';
            out[1] = 'x';
            out[2] = hex[c >> 4];
            out[3] = hex[c & 15];
            out[4] = 0;
        }
        t_puts(out);
    }
    if (n > 100)
        t_puts("...");
    t_puts("\"");
}

static void fail_head(const char *what)
{
    failures++;
    shown++;
    t_puts("FAIL line ");
    t_putu(lineno);
    t_puts(" ");
    t_puts(what);
    t_puts(": ");
}

/* ---- parsing */

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

/* Decode the escapes of s[0..n-1] in place and return the length. */
static unsigned unescape(char *s, unsigned n)
{
    unsigned i = 0, o = 0;

    while (i < n) {
        char c = s[i++];

        if (c == '\\' && i < n) {
            c = s[i++];
            if (c == 't')
                c = '\t';
            else if (c == 'n')
                c = '\n';
            else if (c == 'r')
                c = '\r';
            else if (c == 'x' && i + 1 < n) {
                c = (char)(hexval(s[i]) * 16 + hexval(s[i + 1]));
                i += 2;
            }
        }
        s[o++] = c;
    }
    return o;
}

static unsigned long parse_num(const char *s, unsigned n)
{
    unsigned long v = 0;
    unsigned i = 0;
    int neg = 0;

    if (n && s[0] == '-') {
        neg = 1;
        i = 1;
    }
    for (; i < n; i++) {
        /* v * 10 + digit with additions only: no multiplication helper */
        unsigned long t = v + v;

        t += t;
        t += v;
        t += t;
        v = t + (unsigned)(s[i] - '0');
    }
    return neg ? 0UL - v : v;
}

/* Split `line` into a record. Returns 0, or -1 for a malformed line. */
static int parse_line(struct rec *r)
{
    char *f[5];
    unsigned flen[5];
    unsigned k = 0, i = 0, len;
    char *p;

    len = (unsigned)strlen(line);
    while (len && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        line[--len] = 0;
    f[0] = line;
    for (i = 0; i < len; i++) {
        if (line[i] == '\t') {
            line[i] = 0;
            if (++k > 4)
                return -1;
            f[k] = line + i + 1;
        }
    }
    if (k != 4)
        return -1;
    for (k = 0; k < 4; k++)
        flen[k] = (unsigned)strlen(f[k]);
    flen[4] = (unsigned)strlen(f[4]);

    r->mode = f[0][0];
    r->n = 0;
    if (r->mode == 'N')
        r->n = (unsigned)parse_num(f[0] + 1, flen[0] - 1);
    r->fmt = f[1];
    f[1][unescape(f[1], flen[1])] = 0;
    r->want = f[3];
    r->wantlen = unescape(f[3], flen[3]);
    r->ret = (int)parse_num(f[4], flen[4]);

    /* the arguments */
    r->nw = 0;
    p = f[2];
    while (*p) {
        char *tok = p;
        unsigned tl;
        char kind;

        while (*p && *p != ' ')
            p++;
        tl = (unsigned)(p - tok);
        if (*p == ' ')
            p++;
        if (tl < 2 || tok[1] != ':' || r->nw + 4 > MAXW)
            return -1;
        kind = tok[0];
        tok += 2;
        tl -= 2;
        if (kind == 'i') {
            r->w[r->nw++] = (unsigned)parse_num(tok, tl);
        } else if (kind == 'l') {
            unsigned long v = parse_num(tok, tl);

            r->w[r->nw++] = (unsigned)(v & 0xFFFFu);
            r->w[r->nw++] = (unsigned)(v >> 16);
        } else if (kind == 'd') {
            unsigned j;

            if (tl != 16)
                return -1;
            for (j = 0; j < 4; j++) {
                /* word 3 is the first four hex digits */
                r->w[r->nw + 3 - j] = (unsigned)(hexval(tok[4 * j]) * 4096 + hexval(tok[4 * j + 1]) * 256
                                                 + hexval(tok[4 * j + 2]) * 16 + hexval(tok[4 * j + 3]));
            }
            r->nw += 4;
        } else if (kind == 's' || kind == 'S') {
            const char *sp = 0;
            unsigned j;

            if (kind == 's') {
                tok[unescape(tok, tl)] = 0;
                sp = tok;
            }
            memcpy(&r->w[r->nw], &sp, sizeof sp);
            for (j = 0; j < sizeof sp / 2; j++)
                r->nw++;
        } else {
            return -1;
        }
    }
    return 0;
}

/* The next record of the file: 1, or 0 at its end, or -1 for a bad line. */
static int next_rec(FILE *fp, struct rec *r)
{
    do {
        if (fgets(line, LINEMAX, fp) == 0)
            return 0;
        lineno++;
    } while (line[0] == '#' || line[0] == '\n' || line[0] == '\r');
    return parse_line(r) == 0 ? 1 : -1;
}

/* ---- calling the functions with the words */

#define A0
#define A1  , w[0]
#define A2  A1, w[1]
#define A3  A2, w[2]
#define A4  A3, w[3]
#define A5  A4, w[4]
#define A6  A5, w[5]
#define A7  A6, w[6]
#define A8  A7, w[7]
#define A9  A8, w[8]
#define A10 A9, w[9]
#define A11 A10, w[10]
#define A12 A11, w[11]
#define A13 A12, w[12]
#define A14 A13, w[13]

#define CASES(M) \
    switch (n) { \
    case 0: return M(A0); case 1: return M(A1); case 2: return M(A2); case 3: return M(A3); \
    case 4: return M(A4); case 5: return M(A5); case 6: return M(A6); case 7: return M(A7); \
    case 8: return M(A8); case 9: return M(A9); case 10: return M(A10); case 11: return M(A11); \
    case 12: return M(A12); case 13: return M(A13); case 14: return M(A14); \
    } \
    return -32000

#ifdef DRIVER_MEMCHECK
static int call_sprintf(char *t, const char *f, const unsigned *w, unsigned n)
{
#define M(x) sprintf(t, f x)
    CASES(M);
#undef M
}

static int call_snprintf(char *t, size_t sz, const char *f, const unsigned *w, unsigned n)
{
#define M(x) snprintf(t, sz, f x)
    CASES(M);
#undef M
}

static int call_vsprintf(char *t, const char *f, unsigned *w)
{
    return vsprintf(t, f, (va_list)w);
}

static int call_vsnprintf(char *t, size_t sz, const char *f, unsigned *w)
{
    return vsnprintf(t, sz, f, (va_list)w);
}

#endif

#ifdef DRIVER_STREAMS
static int call_fprintf(FILE *fp, const char *f, const unsigned *w, unsigned n)
{
#define M(x) fprintf(fp, f x)
    CASES(M);
#undef M
}

static int call_printf(const char *f, const unsigned *w, unsigned n)
{
#define M(x) printf(f x)
    CASES(M);
#undef M
}

static int call_vfprintf(FILE *fp, const char *f, unsigned *w)
{
    return vfprintf(fp, f, (va_list)w);
}

static int call_vprintf(const char *f, unsigned *w)
{
    return vprintf(f, (va_list)w);
}
#endif

#ifdef DRIVER_MEMCHECK

/* ---- the memory checks for the functions that write to memory */

#define GUARD 8
#define AREAMAX (GUARD + 1500 + 64 + GUARD)
#define PATTERN 0xA5

static unsigned char area[AREAMAX];

enum { F_SPRINTF, F_VSPRINTF, F_SNPRINTF, F_VSNPRINTF };
static const char *const fnname[] = { "sprintf", "vsprintf", "snprintf", "vsnprintf" };

/* Calls function `which` on the record with room for `room` bytes in the
 * target (`sz` is the n of the n functions), and checks the return value,
 * the output, the terminating null character, and that nothing outside
 * what the function may write changed. */
static void run_one(struct rec *r, int which, unsigned room, unsigned sz)
{
    char *t = (char *)area + GUARD;
    unsigned i, written;
    int ret;
    int ok = 1;

    memset(area, PATTERN, room + 2 * GUARD);
    switch (which) {
    case F_SPRINTF:
        ret = call_sprintf(t, r->fmt, r->w, r->nw);
        break;
    case F_VSPRINTF:
        ret = call_vsprintf(t, r->fmt, r->w);
        break;
    case F_SNPRINTF:
        ret = call_snprintf(t, sz, r->fmt, r->w, r->nw);
        break;
    default:
        ret = call_vsnprintf(t, sz, r->fmt, r->w);
        break;
    }

    /* how much of the output the function may have written, and the null */
    written = r->wantlen;
    if (which >= F_SNPRINTF && written + 1 > sz)
        written = sz ? sz - 1 : 0;
    if (ret != r->ret)
        ok = 0;
    for (i = 0; i < GUARD; i++)
        if (area[i] != PATTERN)
            ok = 0;
    if (memcmp(t, r->want, written) != 0)
        ok = 0;
    if (which < F_SNPRINTF || sz != 0) {
        if (t[written] != 0)
            ok = 0;
        i = written + 1;
    } else {
        i = 0;
    }
    for (; i < room + GUARD; i++)
        if (t[i] != (char)PATTERN)
            ok = 0;
    if (!ok && shown < MAXSHOW) {
        fail_head(fnname[which]);
        t_puts("fmt ");
        put_esc(r->fmt, (unsigned)strlen(r->fmt));
        t_puts(" ret ");
        put_int(ret);
        t_puts(" want ");
        put_int(r->ret);
        t_puts(" out ");
        put_esc(t, written + 1 < room ? written + 1 : room);
        t_puts(" want ");
        put_esc(r->want, r->wantlen);
        if (which >= F_SNPRINTF) {
            t_puts(" n ");
            t_putu(sz);
        }
        t_puts("\r\n");
    } else if (!ok) {
        failures++;
    }
}

/* One record: P runs one of the four functions (all of them with `all`),
 * N the two n functions with n. */
static void run_rec(struct rec *r, int all)
{
    unsigned room;

    if (r->wantlen + 1 + 16 > AREAMAX - 2 * GUARD) {
        fail_head("record");
        t_puts("output too long for the buffer\r\n");
        return;
    }
    if (r->mode == 'N') {
        room = r->wantlen + 1 + 16;
        if (r->n + 16 > room)
            room = r->n + 16;
        if (room > AREAMAX - 2 * GUARD)
            room = AREAMAX - 2 * GUARD;
        run_one(r, F_SNPRINTF, room, r->n);
        run_one(r, F_VSNPRINTF, room, r->n);
        return;
    }
    room = r->wantlen + 1 + 16;
    if (all) {
        run_one(r, F_SPRINTF, room, 0);
        run_one(r, F_VSPRINTF, room, 0);
        run_one(r, F_SNPRINTF, room, room);
        run_one(r, F_VSNPRINTF, room, room);
    } else {
        int which = (int)(lineno & 3);

        run_one(r, which, room, room);
    }
}

#endif /* DRIVER_MEMCHECK */

#endif
