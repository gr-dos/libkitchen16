/* fmt - the conversion engine of the printf family (docs/spec/fmt.md).
 *
 * Integers are converted from two 16-bit words. A double is read from the
 * argument list as four 16-bit words and handled as bits: its digits are
 * made by exact integer work on 16-bit limbs (a big integer of about 1,100
 * bits), never by floating-point arithmetic. The digits are produced twice:
 * once to find out how the value rounds (and so how long the output is),
 * and again to write them. No static storage. */

#include <errno.h>
#include "fmt.h"

#define F_MINUS 1
#define F_PLUS  2
#define F_SPACE 4
#define F_ALT   8
#define F_ZERO  16

#define LIMIT 32767u
#define NONE  0xFFFFu

/* ---- 16-bit multiply and divide that wcc would otherwise do with long
 * arithmetic helpers */

extern unsigned long umul(unsigned a, unsigned b);
#pragma aux umul = "mul dx" parm [ax] [dx] value [dx ax] modify exact [ax dx];

/* (hi:lo) / d, where hi < d */
extern unsigned divq(unsigned hi, unsigned lo, unsigned d);
#pragma aux divq = "div bx" parm [dx] [ax] [bx] value [ax] modify exact [ax dx];

/* ---- output */

typedef struct {
    __k16_out out;
    void *ctx;
    unsigned count;
    char stop, over, limit, err;
} E;

static void put(E *e, int c)
{
    if (e->stop)
        return;
    if (e->count == LIMIT) {
        e->over = 1;
        if (e->limit) {
            e->stop = 1;
            return;
        }
    } else {
        e->count++;
    }
    if (e->out(e->ctx, c)) {
        e->stop = 1;
        e->err = 1;
    }
}

static void rep(E *e, int c, unsigned n)
{
    while (n != 0 && !e->stop) {
        put(e, c);
        n--;
    }
}

/* Padding around a field of `len` characters (a sign or prefix, zeros and
 * digits): the spaces before it, and the spaces after it, and the zeros to
 * add after the sign. zeroable: the 0 flag applies. */
static unsigned pad_left(unsigned fl, unsigned width, unsigned len, int zeroable,
                         unsigned *zeros, unsigned *after)
{
    unsigned pad = width > len ? width - len : 0;

    *zeros = 0;
    *after = 0;
    if (fl & F_MINUS) {
        *after = pad;
        return 0;
    }
    if (zeroable && (fl & F_ZERO)) {
        *zeros = pad;
        return 0;
    }
    return pad;
}

/* ---- exact digits of a double */

#define NLIMB 68        /* 16 * 68 >= 1,074 bits of fraction */
#define NCHUNK 78       /* 4 digits each: 309 digits of integer part */

typedef struct {
    unsigned L[NLIMB];  /* the fraction, left-aligned in nl limbs, or scratch */
    unsigned ch[NCHUNK];/* integer part, 4 digits a chunk, most significant first */
    unsigned ci;        /* next chunk */
    unsigned cur;       /* the digits of the current group, curn of them left */
    int curn;
    int pend;           /* a digit read ahead, or -1 */
    int lo, hi, nl;     /* the nonzero limbs of the fraction; lo > hi: none */
    unsigned ni;        /* digits of the integer part, 0 if it is 0 */
    unsigned nieff;     /* the same, at least 1 */
} Gen;

typedef struct {
    unsigned h;         /* index of the last digit that is not a 9, or NONE */
    unsigned lastnz;    /* index of the last nonzero digit, or NONE */
    char up;            /* round up */
    char carry;         /* up, and every kept digit is a 9 */
} Rnd;

static unsigned pw10(int n)
{
    return n >= 4 ? 1000u : n == 3 ? 100u : n == 2 ? 10u : 1u;
}

/* The digits of the exact value (m[3..0] is the 53-bit mantissa, times 2 to
 * the exp). sig: no synthetic leading 0 digit for an integer part of 0. */
static void ginit(Gen *g, const unsigned *m, int exp, int sig)
{
    unsigned *L = g->L;
    int n, i, c;
    unsigned rem, q;

    g->pend = -1;
    g->curn = 0;
    g->ci = NCHUNK;
    g->lo = 1;
    g->hi = 0;
    g->nl = 0;
    g->ni = 0;

    /* the integer part, in L */
    for (i = 0; i < NLIMB; i++)
        L[i] = 0;
    n = 0;
    if (exp >= 0) {
        int ls = exp >> 4, bs = exp & 15;

        for (i = 0; i < 4; i++)
            L[ls + i] = m[i];
        if (bs) {
            for (i = ls + 4; i > ls; i--)
                L[i] = (L[i] << bs) | (L[i - 1] >> (16 - bs));
            L[ls] <<= bs;
            n = ls + 5;
        } else {
            n = ls + 4;
        }
    } else if (exp > -53) {
        int k = -exp, ls = k >> 4, bs = k & 15;

        for (i = 0; i < 4; i++) {
            unsigned lowp = i + ls < 4 ? m[i + ls] : 0;
            unsigned highp = i + ls + 1 < 4 ? m[i + ls + 1] : 0;

            L[i] = bs ? (lowp >> bs) | (highp << (16 - bs)) : lowp;
        }
        n = 4;
    }
    while (n > 0 && L[n - 1] == 0)
        n--;

    c = NCHUNK;
    while (n > 0) {
        rem = 0;
        for (i = n - 1; i >= 0; i--) {
            q = divq(rem, L[i], 10000u);
            rem = L[i] - q * 10000u;
            L[i] = q;
        }
        while (n > 0 && L[n - 1] == 0)
            n--;
        g->ch[--c] = rem;
    }
    if (c < NCHUNK) {
        unsigned top = g->ch[c];

        g->cur = top;
        g->curn = top >= 1000 ? 4 : top >= 100 ? 3 : top >= 10 ? 2 : 1;
        g->ni = (unsigned)(NCHUNK - c - 1) * 4u + (unsigned)g->curn;
        g->ci = c + 1;
    } else if (!sig) {
        g->cur = 0;
        g->curn = 1;
    }
    g->nieff = g->ni ? g->ni : 1;

    /* the fraction, left-aligned */
    if (exp < 0) {
        int k = -exp, s;
        int nl = (k + 15) >> 4;

        for (i = 0; i < NLIMB; i++)
            L[i] = 0;
        for (i = 0; i < 4 && i < nl; i++) {
            int lowbit = i * 16;
            unsigned v = m[i];

            if (k < lowbit + 16)
                v &= (1u << (k - lowbit)) - 1;
            L[i] = v;
        }
        s = nl * 16 - k;
        if (s) {
            for (i = nl - 1; i > 0; i--)
                L[i] = (L[i] << s) | (L[i - 1] >> (16 - s));
            L[0] <<= s;
        }
        g->nl = nl;
        g->hi = nl - 1;
        g->lo = 0;
        while (g->hi >= 0 && L[g->hi] == 0)
            g->hi--;
        while (g->lo <= g->hi && L[g->lo] == 0)
            g->lo++;
    }
}

/* The next four digits of the fraction */
static void gengroup(Gen *g)
{
    unsigned *L = g->L;
    unsigned long t;
    unsigned carry = 0;
    int i;

    for (i = g->lo; i <= g->hi; i++) {
        t = umul(L[i], 10000u) + carry;
        L[i] = (unsigned)t;
        carry = (unsigned)(t >> 16);
    }
    if (g->hi < g->nl - 1) {
        if (carry) {
            g->hi++;
            L[g->hi] = carry;
        }
        g->cur = 0;
    } else {
        g->cur = carry;
    }
    g->curn = 4;
    while (g->lo <= g->hi && L[g->lo] == 0)
        g->lo++;
}

static int exh(const Gen *g)
{
    return g->pend < 0 && g->curn == 0 && g->ci >= NCHUNK && g->lo > g->hi;
}

static int nd(Gen *g)
{
    int d;
    unsigned p;

    if (g->pend >= 0) {
        d = g->pend;
        g->pend = -1;
        return d;
    }
    if (g->curn == 0) {
        if (g->ci < NCHUNK) {
            g->cur = g->ch[g->ci++];
            g->curn = 4;
        } else if (g->lo <= g->hi) {
            gengroup(g);
        } else {
            return 0;
        }
    }
    p = pw10(g->curn);
    d = g->cur / p;
    g->cur -= d * p;
    g->curn--;
    return d;
}

/* Skips the leading zeros of the fraction when the integer part is 0.
 * Returns the decimal exponent of the first nonzero digit (0 for a zero). */
static int firstsig(Gen *g)
{
    int z = 0, d;

    if (g->ni)
        return (int)g->ni - 1;
    for (;;) {
        if (exh(g))
            return 0;
        d = nd(g);
        if (d) {
            g->pend = d;
            return -(z + 1);
        }
        z++;
    }
}

/* Reads n digits and decides how the value rounds to them. */
static void scan(Gen *g, unsigned n, Rnd *r)
{
    unsigned i = 0;
    int d, lastd = 0;

    r->h = NONE;
    r->lastnz = NONE;
    r->up = 0;
    while (i < n) {
        if (exh(g))
            break;
        d = nd(g);
        if (d != 9)
            r->h = i;
        if (d)
            r->lastnz = i;
        lastd = d;
        i++;
    }
    if (i == n) {
        d = nd(g);
        if (d > 5) {
            r->up = 1;
        } else if (d == 5) {
            int sticky = 0;

            while (!exh(g))
                if (nd(g)) {
                    sticky = 1;
                    break;
                }
            if (sticky || (lastd & 1))
                r->up = 1;
        }
    }
    r->carry = r->up && r->h == NONE;
}

/* The ith digit as rounded. sig: a carry gives 1000... of the same length */
static int rdig(Gen *g, const Rnd *r, unsigned i, int sig)
{
    int d = nd(g);

    if (!r->up)
        return d;
    if (r->carry)
        return sig && i == 0;
    if (i < r->h)
        return d;
    if (i == r->h)
        return d + 1;
    return 0;
}

static void put_exp(E *e, int upper, int x)
{
    unsigned a = x < 0 ? -x : x;

    put(e, upper ? 'E' : 'e');
    put(e, x < 0 ? '-' : '+');
    if (a >= 100) {
        put(e, '0' + a / 100);
        a %= 100;
    }
    put(e, '0' + a / 10);
    put(e, '0' + a % 10);
}

static void put_text(E *e, const char *s)
{
    while (*s)
        put(e, *s++);
}

/* One floating conversion; w is the double's four words, low first. */
static void do_float(E *e, unsigned fl, unsigned width, int prec, int conv,
                     const unsigned *w)
{
    Gen g;
    Rnd r;
    unsigned m[4];
    int exp, bexp, lower = conv | 0x20, upper = conv != lower;
    int alt = (fl & F_ALT) != 0;
    char sign = (w[3] & 0x8000u) ? '-' : (fl & F_PLUS) ? '+' : (fl & F_SPACE) ? ' ' : 0;
    unsigned sl = sign != 0;
    unsigned p, len, before, zeros, after, i, N;
    int X = 0, X0, sig, fstyle, dot;
    unsigned Fd = 0, nd_ = 0, lz;

    bexp = (w[3] >> 4) & 0x7FF;
    if (bexp == 0x7FF) {
        int isnan = ((w[3] & 0xF) | w[2] | w[1] | w[0]) != 0;
        const char *t = isnan ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf");

        before = pad_left(fl, width, sl + 3, 0, &zeros, &after);
        rep(e, ' ', before);
        if (sign)
            put(e, sign);
        put_text(e, t);
        rep(e, ' ', after);
        return;
    }
    m[0] = w[0];
    m[1] = w[1];
    m[2] = w[2];
    m[3] = (w[3] & 0xF) | (bexp ? 0x10 : 0);
    exp = bexp ? bexp - 1075 : -1074;

    sig = lower != 'f';
    if (lower == 'g')
        p = prec < 0 ? 6 : prec == 0 ? 1 : (unsigned)prec;
    else
        p = prec < 0 ? 6 : (unsigned)prec;

    /* first pass: how the value rounds, and from that the length */
    ginit(&g, m, exp, sig);
    X0 = sig ? firstsig(&g) : 0;
    if (lower == 'f')
        N = g.nieff + p;
    else if (lower == 'e')
        N = p + 1;
    else
        N = p;
    scan(&g, N, &r);
    X = X0 + r.carry;

    fstyle = lower == 'f';
    dot = alt;
    if (lower == 'g') {
        unsigned L = r.up ? (r.carry ? 0 : r.h) : r.lastnz;

        if (L == NONE)
            L = 0;
        fstyle = (int)p > X && X >= -4;
        if (fstyle) {
            if (alt)
                Fd = p - 1 - X;
            else if (X >= 0)
                Fd = (int)L > X ? L - X : 0;
            else
                Fd = L - X;
            dot = Fd > 0 || alt;
            len = (X >= 0 ? (unsigned)X + 1 : 1) + dot + Fd;
        } else {
            nd_ = alt ? p : L + 1;
            dot = nd_ > 1 || alt;
            len = nd_ + dot + 2 + ((X >= 100 || X <= -100) ? 3 : 2);
        }
    } else if (lower == 'f') {
        dot = p > 0 || alt;
        len = g.nieff + r.carry + dot + p;
    } else {
        dot = p > 0 || alt;
        len = 1 + dot + p + 2 + ((X >= 100 || X <= -100) ? 3 : 2);
    }

    before = pad_left(fl, width, sl + len, 1, &zeros, &after);
    rep(e, ' ', before);
    if (sign)
        put(e, sign);
    rep(e, '0', zeros);

    /* second pass: the digits */
    ginit(&g, m, exp, sig);
    if (sig)
        firstsig(&g);
    if (lower == 'f') {
        if (r.carry)
            put(e, '1');
        for (i = 0; i < N && !e->stop; i++) {
            put(e, '0' + rdig(&g, &r, i, 0));
            if (i + 1 == g.nieff && dot)
                put(e, '.');
        }
    } else if (fstyle) {
        /* g, in f style: X + 1 + Fd digits, after (X < 0) 0. and zeros */
        unsigned nint = X >= 0 ? (unsigned)X + 1 : 1;

        i = 0;
        if (X >= 0) {
            for (; i < nint && !e->stop; i++)
                put(e, '0' + rdig(&g, &r, i, 1));
        } else {
            put(e, '0');
        }
        if (dot)
            put(e, '.');
        if (X < 0) {
            lz = (unsigned)(-X - 1);
            rep(e, '0', lz);
            for (i = 0; i < Fd - lz && !e->stop; i++)
                put(e, '0' + rdig(&g, &r, i, 1));
        } else {
            for (; i < nint + Fd && !e->stop; i++)
                put(e, '0' + rdig(&g, &r, i, 1));
        }
    } else {
        unsigned cnt = lower == 'g' ? nd_ : p + 1;

        for (i = 0; i < cnt && !e->stop; i++) {
            put(e, '0' + rdig(&g, &r, i, 1));
            if (i == 0 && dot)
                put(e, '.');
        }
        put_exp(e, upper, X);
    }
    rep(e, ' ', after);
}

/* ---- integers, strings */

static void do_int(E *e, unsigned fl, unsigned width, int prec, int conv,
                   unsigned lo, unsigned hi)
{
    char buf[12];
    int nb = 0;
    char sign = 0, pre0 = 0, pre1 = 0;
    unsigned zeros, after, before, len, z;
    unsigned prc = prec < 0 ? 1 : (unsigned)prec;

    if (conv == 'd' || conv == 'i') {
        if (hi & 0x8000u) {
            sign = '-';
            lo = -lo;
            hi = ~hi + (lo == 0);
        } else if (fl & F_PLUS) {
            sign = '+';
        } else if (fl & F_SPACE) {
            sign = ' ';
        }
    }
    if (conv == 'o') {
        while (lo | hi) {
            buf[nb++] = '0' + (lo & 7);
            lo = (lo >> 3) | (hi << 13);
            hi >>= 3;
        }
    } else if (conv == 'x' || conv == 'X') {
        const char *dg = conv == 'x' ? "0123456789abcdef" : "0123456789ABCDEF";

        if ((lo | hi) && (fl & F_ALT)) {
            pre0 = '0';
            pre1 = conv;
        }
        while (lo | hi) {
            buf[nb++] = dg[lo & 15];
            lo = (lo >> 4) | (hi << 12);
            hi >>= 4;
        }
    } else {
        while (hi) {
            unsigned q = hi / 10, r = hi - q * 10, ql;

            ql = divq(r, lo, 10);
            buf[nb++] = '0' + (lo - ql * 10);
            lo = ql;
            hi = q;
        }
        while (lo) {
            unsigned q = lo / 10;

            buf[nb++] = '0' + (lo - q * 10);
            lo = q;
        }
    }
    z = prc > (unsigned)nb ? prc - nb : 0;
    if (conv == 'o' && (fl & F_ALT) && z == 0 && (nb == 0 || buf[nb - 1] != '0'))
        z = 1;
    len = (sign != 0) + (pre0 ? 2 : 0) + z + nb;
    before = pad_left(fl, width, len, prec < 0, &zeros, &after);
    z += zeros;
    rep(e, ' ', before);
    if (sign)
        put(e, sign);
    if (pre0) {
        put(e, pre0);
        put(e, pre1);
    }
    rep(e, '0', z);
    while (nb > 0 && !e->stop)
        put(e, buf[--nb]);
    rep(e, ' ', after);
}

static void do_str(E *e, unsigned fl, unsigned width, int prec, const char *s)
{
    unsigned len = 0, zeros, after, before;
    const char *p;

    if (s == 0)
        s = "(null)";
    p = s;
    while (*p && (prec < 0 || len < (unsigned)prec)) {
        p++;
        len++;
    }
    before = pad_left(fl, width, len, 0, &zeros, &after);
    rep(e, ' ', before);
    while (len-- && !e->stop)
        put(e, *s++);
    rep(e, ' ', after);
}

/* ---- the format */

static int isdig(int c)
{
    return c >= '0' && c <= '9';
}

static unsigned num(const char **pf)
{
    const char *f = *pf;
    unsigned n = 0;

    while (isdig(*f)) {
        if (n > 3276)
            n = LIMIT;
        else
            n = n * 10 + (*f - '0');
        if (n > LIMIT)
            n = LIMIT;
        f++;
    }
    *pf = f;
    return n;
}

static unsigned clampint(int v)
{
    if (v < 0)
        return v == -32767 - 1 ? LIMIT : (unsigned)(-v);
    return (unsigned)v;
}

int __cdecl __kitchen16_fmt(__k16_out out, void *ctx, const char *f, va_list ap,
                            int limit)
{
    E e;

    e.out = out;
    e.ctx = ctx;
    e.count = 0;
    e.stop = e.over = e.err = 0;
    e.limit = (char)limit;

    while (*f && !e.stop) {
        const char *start;
        unsigned fl = 0, width = 0;
        int prec = -1, starw = 0, starp = 0, mod = 0, conv, bad = 0;

        if (*f != '%') {
            put(&e, *f++);
            continue;
        }
        start = f++;
        for (;; f++) {
            if (*f == '-')
                fl |= F_MINUS;
            else if (*f == '+')
                fl |= F_PLUS;
            else if (*f == ' ')
                fl |= F_SPACE;
            else if (*f == '#')
                fl |= F_ALT;
            else if (*f == '0')
                fl |= F_ZERO;
            else
                break;
        }
        if (*f == '*') {
            starw = 1;
            f++;
            if (isdig(*f))
                bad = 1;
        } else {
            width = num(&f);
        }
        if (!bad && *f == '.') {
            f++;
            if (*f == '*') {
                starp = 1;
                f++;
                if (isdig(*f))
                    bad = 1;
            } else {
                prec = (int)num(&f);
            }
        }
        if (!bad && (*f == 'h' || *f == 'l' || *f == 'L')) {
            mod = *f++;
            if (*f == 'h' || *f == 'l' || *f == 'L')
                bad = 1;
        }
        conv = *f;
        if (!bad) {
            switch (conv) {
            case 'd': case 'i': case 'u': case 'o': case 'x': case 'X':
            case 'e': case 'E': case 'f': case 'g': case 'G':
                break;
            case 'c': case 's':
                bad = mod == 'l';
                break;
            case '%':
                bad = f != start + 1;
                break;
            default:
                bad = 1;
            }
        }
        if (bad) {
            /* copied as written, through the offending character */
            while (start <= f && *start) {
                put(&e, *start++);
            }
            if (*f)
                f++;
            continue;
        }
        f++;

        if (conv == '%') {
            put(&e, '%');
            continue;
        }
        if (starw) {
            int v = va_arg(ap, int);

            if (v < 0)
                fl |= F_MINUS;
            width = clampint(v);
            if (width > LIMIT)
                width = LIMIT;
        }
        if (starp) {
            int v = va_arg(ap, int);

            prec = v < 0 ? -1 : v;
        }
        switch (conv) {
        case 'd': case 'i': case 'u': case 'o': case 'x': case 'X': {
            unsigned lo = va_arg(ap, unsigned), hi;

            if (mod == 'l')
                hi = va_arg(ap, unsigned);
            else
                hi = (conv == 'd' || conv == 'i') && (lo & 0x8000u) ? 0xFFFFu : 0;
            do_int(&e, fl, width, prec, conv, lo, hi);
            break;
        }
        case 'c': {
            unsigned c = va_arg(ap, unsigned) & 0xFF;
            unsigned zeros, after, before;

            before = pad_left(fl, width, 1, 0, &zeros, &after);
            rep(&e, ' ', before);
            put(&e, c);
            rep(&e, ' ', after);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);

            do_str(&e, fl, width, prec, s);
            break;
        }
        default: {
            unsigned w[4];

            w[0] = va_arg(ap, unsigned);
            w[1] = va_arg(ap, unsigned);
            w[2] = va_arg(ap, unsigned);
            w[3] = va_arg(ap, unsigned);
            do_float(&e, fl, width, prec, conv, w);
        }
        }
    }
    if (e.err)
        return -1;
    if (e.over) {
        errno = ERANGE;
        return -1;
    }
    return (int)e.count;
}
