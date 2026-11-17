#include <stdio.h>
#include "data.h"
#include "testio.h"
#include "hex.h"

#define MAXLINE 2100
#define MAXREPORT 20

static FILE *fp;
static const char *fname;
static char line[MAXLINE];
static char buf[MAXLINE];
static unsigned lineno;
static unsigned failures;

static void where(unsigned n)
{
    t_puts("not ok ");
    t_puts(fname);
    t_puts(":");
    t_putu(n);
    t_puts(": ");
}

int data_open(const char *name)
{
    fname = name;
    fp = fopen(name, "r");
    if (fp == 0) {
        t_puts("cannot open ");
        t_puts(name);
        t_puts("\r\n");
        failures++;
        return 1;
    }
    return 0;
}

static int hexval(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

/* Parse the line in LINE into L. Returns 0 if it is malformed. */
static int parse(struct dline *l)
{
    char *p = line;
    char *q;
    int i;
    int v;
    unsigned o = 0;

    l->nan = 0;
    l->w[0] = l->w[1] = l->w[2] = l->w[3] = 0;
    if (p[0] == 'n' && p[1] == 'a' && p[2] == 'n' && (p[3] == '+' || p[3] == '-')) {
        l->nan = p[3] == '+' ? 1 : -1;
        p += 4;
    } else {
        for (i = 0; i < 16; i++) {
            v = hexval(p[i]);
            if (v < 0)
                return 0;
            l->w[3 - i / 4] = (l->w[3 - i / 4] << 4) | v;
        }
        p += 16;
    }
    if (*p++ != ' ')
        return 0;
    if (*p < '0' || *p > '9')
        return 0;
    while (*p >= '0' && *p <= '9')
        o = o * 10 + (*p++ - '0');
    l->off = o;
    if (*p++ != ' ')
        return 0;
    if (*p == 'R')
        l->erange = 1;
    else if (*p == '-')
        l->erange = 0;
    else
        return 0;
    p++;
    if (*p == ' ')
        p++;
    else if (*p != '\0')
        return 0;
    /* the string, unescaped */
    q = buf;
    while (*p) {
        if (*p == '\\') {
            int h = hexval(p[2]);
            int g = p[1] == 'x' ? hexval(p[3]) : -1;

            if (h < 0 || g < 0)
                return 0;
            *q++ = (char)(h * 16 + g);
            p += 4;
        } else {
            *q++ = *p++;
        }
    }
    *q = '\0';
    l->str = buf;
    return 1;
}

int data_next(struct dline *l)
{
    unsigned n;

    for (;;) {
        if (fp == 0 || fgets(line, sizeof line, fp) == 0)
            return 0;
        lineno++;
        n = 0;
        while (line[n])
            n++;
        if (n == sizeof line - 1 && line[n - 1] != '\n') {
            if (failures < MAXREPORT) {
                where(lineno);
                t_puts("line too long\r\n");
            }
            failures++;
            continue;
        }
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = '\0';
        if (n == 0 || line[0] == '#')
            continue;
        l->lineno = lineno;
        if (parse(l))
            return 1;
        if (failures < MAXREPORT) {
            where(lineno);
            t_puts("malformed data line\r\n");
        }
        failures++;
    }
}

int data_same(const struct dline *l, const unsigned short *w)
{
    if (l->nan) {
        int isnan = (w[3] & 0x7FF0) == 0x7FF0 && ((w[3] & 0x000F) | w[2] | w[1] | w[0]) != 0;

        return isnan && ((w[3] & 0x8000) != 0) == (l->nan < 0);
    }
    return w[0] == l->w[0] && w[1] == l->w[1] && w[2] == l->w[2] && w[3] == l->w[3];
}

void data_fail(const struct dline *l, const unsigned short *w, unsigned off, int err)
{
    int i;

    if (failures++ >= MAXREPORT)
        return;
    where(l->lineno);
    t_puts("got ");
    for (i = 3; i >= 0; i--)
        t_puthex16(w[i]);
    t_puts(" off ");
    t_putu(off);
    t_puts(" errno ");
    t_putu((unsigned)err);
    t_puts(", expected ");
    if (l->nan) {
        t_puts(l->nan > 0 ? "nan+" : "nan-");
    } else {
        for (i = 3; i >= 0; i--)
            t_puthex16(l->w[i]);
    }
    t_puts(" off ");
    t_putu(l->off);
    t_puts(l->erange ? " ERANGE\r\n" : " no error\r\n");
}

void data_fail_msg(const struct dline *l, const char *msg)
{
    if (failures++ >= MAXREPORT)
        return;
    where(l->lineno);
    t_puts(msg);
    t_puts("\r\n");
}

int data_done(void)
{
    if (fp)
        fclose(fp);
    if (failures == 0) {
        t_puts("ok\r\n");
        return 0;
    }
    if (failures > MAXREPORT) {
        t_putu(failures);
        t_puts(" failures\r\n");
    }
    return 1;
}
