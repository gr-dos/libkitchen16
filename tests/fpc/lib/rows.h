/* Reading a data file of tests/fpc (see tests/fpc/gen for the formats): one
 * row per line, fields separated by spaces, lines starting with # ignored.
 * Hexadecimal fields are parsed by hand into 16-bit words, least
 * significant first, with no floating-point operation, so that nothing here
 * calls a helper under test. */

#ifndef FPC_ROWS_H
#define FPC_ROWS_H

#include <stdio.h>
#include "report.h"

typedef unsigned short u16;

static void put_hex(unsigned w)
{
    char b[5];
    int i;

    for (i = 3; i >= 0; i--) {
        b[i] = "0123456789ABCDEF"[w & 15];
        w >>= 4;
    }
    b[4] = '\0';
    t_puts(b);
}

/* put the words w[n-1] .. w[0] (the most significant first) */
static void put_words(const unsigned short *w, int n)
{
    while (n--)
        put_hex(w[n]);
}

static FILE *data_fp;
static char row[200];
static unsigned rowno;

static int open_data(const char *name)
{
    data_fp = fopen(name, "r");
    if (data_fp == NULL) {
        t_puts("not ok: cannot open ");
        t_puts(name);
        t_puts("\r\n");
        return 0;
    }
    return 1;
}

/* the next row in `row`, without its line end; 0 at the end of the file */
static int next_row(void)
{
    for (;;) {
        char *p;

        if (fgets(row, sizeof row, data_fp) == NULL)
            return 0;
        rowno++;
        for (p = row; *p && *p != '\r' && *p != '\n'; p++)
            ;
        *p = '\0';
        if (row[0] != '\0' && row[0] != '#')
            return 1;
    }
}

static const char *skip_sp(const char *p)
{
    while (*p == ' ')
        p++;
    return p;
}

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

/* a field of `nwords` 16-bit words (4 digits each) into w[nwords-1] .. w[0];
 * the field "N" (a NaN of any kind) sets *isnan; returns the rest of the
 * row, or NULL if the field is bad */
static const char *get_field(const char *p, u16 *w, int nwords, int *isnan)
{
    int i, j;

    p = skip_sp(p);
    *isnan = 0;
    if (p[0] == 'N' && (p[1] == '\0' || p[1] == ' ')) {
        *isnan = 1;
        for (i = 0; i < nwords; i++)
            w[i] = 0;
        return p + 1;
    }
    for (i = nwords - 1; i >= 0; i--) {
        unsigned v = 0;

        for (j = 0; j < 4; j++) {
            int d = hexval(*p);

            if (d < 0)
                return NULL;
            v = (v << 4) | (unsigned)d;
            p++;
        }
        w[i] = v;
    }
    return p;
}

/* one character field */
static const char *get_char(const char *p, char *c)
{
    p = skip_sp(p);
    *c = *p;
    return *p ? p + 1 : NULL;
}

static void bad_row(void)
{
    nfail++;
    if (nfail <= MAXPRINT) {
        t_puts("not ok: bad data row ");
        t_putu(rowno);
        t_puts("\r\n");
    }
}

/* the failure line for the current row: TAG, row number, the row, "got" and
 * the words that the program found, most significant first */
static void fail_row(const char *tag, const u16 *got, int ngot)
{
    if (begin_fail(tag)) {
        t_putu(rowno);
        t_puts(": ");
        t_puts(row);
        t_puts(" got ");
        put_words(got, ngot);
        t_puts("\r\n");
    }
}

static int is_nan64(const u16 *w)
{
    return (w[3] & 0x7FF0) == 0x7FF0 && ((w[3] & 0x000F) | w[2] | w[1] | w[0]) != 0;
}

static int is_nan32(const u16 *w)
{
    return (w[1] & 0x7F80) == 0x7F80 && ((w[1] & 0x007F) | w[0]) != 0;
}

/* copy n words of a volatile object */
static void grab(u16 *dst, const volatile u16 *src, int n)
{
    int i;

    for (i = 0; i < n; i++)
        dst[i] = src[i];
}

/* poke n words into a volatile object */
static void poke(volatile u16 *dst, const u16 *src, int n)
{
    int i;

    for (i = 0; i < n; i++)
        dst[i] = src[i];
}

/* does `got` (n words) match `want`, which is "any NaN" if isnan? */
static int same_bits(const u16 *got, const u16 *want, int n, int isnan)
{
    int i;

    nchecks++;
    if (isnan)
        return n == 4 ? is_nan64(got) : is_nan32(got);
    for (i = 0; i < n; i++)
        if (got[i] != want[i])
            return 0;
    return 1;
}

/* a program uses some of the functions above only; referring to all of them
 * here keeps -we from refusing the others as unreferenced */
void fpc_keep_helpers(void);
void fpc_keep_helpers(void)
{
    (void)get_char;
    (void)grab;
    (void)poke;
    (void)same_bits;
    (void)bad_row;
    (void)fail_row;
    (void)is_nan32;
}

#endif
