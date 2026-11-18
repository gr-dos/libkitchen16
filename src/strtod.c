/* strtod: decimal text to the nearest double, exactly (docs/spec/conv.md).
 *
 * The text is read into a big integer M (at most 800 digits, plus one more
 * digit 1 standing for any non-zero digit beyond them, which can only break
 * a tie) and a decimal exponent E: the value is M * 10**E = M * 5**E *
 * 2**E. That is a quotient A / B of two big integers times a power of two
 * (E >= 0: A = M * 5**E, B = 1; E < 0: A = M, B = 5**-E). A and B are
 * shifted to the same bit length, the quotient is developed to 56 bits by
 * restoring division, with the remainder as a sticky bit, and rounded to 53
 * bits, or to fewer in the subnormal range, ties to even. The 64-bit
 * result is built from words: no floating-point operation is used, so no
 * helper is linked. Static storage: two big integers of 336 bytes.
 * Not reentrant. Stack: under 100 bytes beyond the callers'. */

#include <errno.h>
#include <kitchen16.h>

#define NW   168                /* words of a big integer: 2,688 bits */
#define MAXD 800                /* significant digits kept */

typedef unsigned short u16;
typedef unsigned long u32;

struct big {
    unsigned n;                 /* words in use; w[n-1] != 0 */
    u16 w[NW];
};
#define BP struct big __near *

static struct big X, Y;
#define PX ((BP)&X)
#define PY ((BP)&Y)

static union {
    double d;
    u16 w[4];
} res;

/* b = b * 10**n + add (f == 0) or b * 5**n + add (f == 1); n <= 4 or 6. */
static void mulp(BP b, int f, int n, u16 add)
{
    u32 t;
    u16 carry = add;
    unsigned i;
    int j;

    for (i = 0; i < b->n; i++) {
        t = b->w[i];
        if (f) {
            for (j = n; j; j--)
                t = (t << 2) + t;
        } else {
            for (j = n; j; j--)
                t = (t << 3) + (t << 1);
        }
        t += carry;
        b->w[i] = (u16)t;
        carry = (u16)(t >> 16);
    }
    if (carry)
        b->w[b->n++] = carry;
}

static unsigned bitlen(BP b)
{
    unsigned n = b->n, k = 0;
    u16 top;

    if (!n)
        return 0;
    top = b->w[n - 1];
    while (top) {
        k++;
        top >>= 1;
    }
    return (n - 1) * 16 + k;
}

static void shl(BP b, unsigned s)
{
    unsigned ws = s >> 4, bs = s & 15, i, n = b->n;
    u16 hi;

    if (!n)
        return;
    if (bs) {
        hi = b->w[n - 1] >> (16 - bs);
        for (i = n - 1; i; i--)
            b->w[i] = (b->w[i] << bs) | (b->w[i - 1] >> (16 - bs));
        b->w[0] <<= bs;
        if (hi)
            b->w[n++] = hi;
    }
    if (ws) {
        for (i = n; i--;)
            b->w[i + ws] = b->w[i];
        for (i = 0; i < ws; i++)
            b->w[i] = 0;
        n += ws;
    }
    b->n = n;
}

static void shl1(BP b)
{
    unsigned i, n = b->n;
    u16 c = 0, v;

    for (i = 0; i < n; i++) {
        v = b->w[i];
        b->w[i] = (v << 1) | c;
        c = v >> 15;
    }
    if (c)
        b->w[b->n++] = 1;
}

/* a >= b? */
static int ge(BP a, BP b)
{
    unsigned i;

    if (a->n != b->n)
        return a->n > b->n;
    for (i = a->n; i--;)
        if (a->w[i] != b->w[i])
            return a->w[i] > b->w[i];
    return 1;
}

/* a -= b, a >= b */
static void sub(BP a, BP b)
{
    unsigned i, n = a->n;
    u16 x, y, br = 0;

    for (i = 0; i < n; i++) {
        x = a->w[i];
        y = i < b->n ? b->w[i] : 0;
        a->w[i] = x - y - br;
        br = (x < y) || (br && x == y);
    }
    while (n && !a->w[n - 1])
        n--;
    a->n = n;
}

static double setbits(unsigned w3, unsigned w2, unsigned w1, unsigned w0)
{
    res.w[3] = w3;
    res.w[2] = w2;
    res.w[1] = w1;
    res.w[0] = w0;
    return res.d;
}

#define SIGN(neg) ((neg) ? 0x8000u : 0u)

#define lower(c) ((c) | 32)

static int isword(int c)
{
    return (c >= '0' && c <= '9') || c == '_' || (lower(c) >= 'a' && lower(c) <= 'z');
}

double __cdecl strtod(const char *nptr, char **endptr)
{
    const char *p = nptr, *q, *e;
    int neg = 0, c, seenpt = 0, anydig = 0, sticky = 0, sg, d;
    long E = 0, ex, dp;
    unsigned nd = 0, nb = 0, wd = 0, bl, la, lb, t, r, guard, low, inexact;
    int i, pp, e2, we;
    u16 qw[4];
    double ret;

    while ((unsigned char)*p == ' ' || ((unsigned char)*p >= 9 && (unsigned char)*p <= 13))
        p++;
    if (*p == '+' || *p == '-') {
        neg = *p == '-';
        p++;
    }
    c = (unsigned char)*p;
    e = 0;

    if (lower(c) == 'i') {
        if (lower((unsigned char)p[1]) == 'n' && lower((unsigned char)p[2]) == 'f') {
            e = p + 3;
            if (lower((unsigned char)p[3]) == 'i' && lower((unsigned char)p[4]) == 'n'
                && lower((unsigned char)p[5]) == 'i' && lower((unsigned char)p[6]) == 't'
                && lower((unsigned char)p[7]) == 'y')
                e = p + 8;
            if (endptr)
                *endptr = (char *)e;
            return setbits(0x7FF0 | SIGN(neg), 0, 0, 0);
        }
    } else if (lower(c) == 'n') {
        if (lower((unsigned char)p[1]) == 'a' && lower((unsigned char)p[2]) == 'n') {
            e = p + 3;
            if (p[3] == '(') {
                q = p + 4;
                while (isword((unsigned char)*q))
                    q++;
                if (*q == ')')
                    e = q + 1;
            }
            if (endptr)
                *endptr = (char *)e;
            return setbits(0x7FF8 | SIGN(neg), 0, 0, 0);
        }
    }

    /* digits, one point */
    q = p;
    for (;;) {
        c = (unsigned char)*q;
        if (c >= '0' && c <= '9') {
            anydig = 1;
            d = c - '0';
            if (nd == 0 && d == 0) {
                if (seenpt)
                    E--;
            } else if (nd < MAXD) {
                nd++;
                if (seenpt)
                    E--;
                wd = wd * 10 + d;
                if (++nb == 4) {
                    mulp(PX, 0, 4, wd);
                    nb = 0;
                    wd = 0;
                }
            } else {
                if (!seenpt)
                    E++;
                if (d)
                    sticky = 1;
            }
        } else if (c == '.' && !seenpt) {
            seenpt = 1;
        } else
            break;
        q++;
    }
    if (!anydig) {
        if (endptr)
            *endptr = (char *)nptr;
        return setbits(0, 0, 0, 0);
    }
    e = q;
    if (lower(c) == 'e') {
        const char *s = q + 1;

        sg = 0;
        if (*s == '+' || *s == '-') {
            sg = *s == '-';
            s++;
        }
        if (*s >= '0' && *s <= '9') {
            ex = 0;
            while (*s >= '0' && *s <= '9') {
                if (ex < 100000000L)
                    ex = (ex << 3) + (ex << 1) + (*s - '0');
                s++;
            }
            E += sg ? -ex : ex;
            e = s;
        }
    }
    if (endptr)
        *endptr = (char *)e;

    if (nd == 0) {
        X.n = 0;
        return setbits(SIGN(neg), 0, 0, 0);
    }
    if (sticky) {
        if (nb == 4) {
            mulp(PX, 0, 4, wd);
            nb = 0;
            wd = 0;
        }
        wd = wd * 10 + 1;
        nb++;
        E--;
        nd++;
    }
    if (nb)
        mulp(PX, 0, nb, wd);

    dp = (long)nd + E;
    if (dp >= 310) {
        X.n = 0;
        errno = ERANGE;
        return setbits(0x7FF0 | SIGN(neg), 0, 0, 0);
    }
    if (dp <= -325) {
        X.n = 0;
        errno = ERANGE;
        return setbits(SIGN(neg), 0, 0, 0);
    }

    /* X = M; Y = 1; then multiply X (E >= 0) or Y (E < 0) by 5**|E| */
    Y.w[0] = 1;
    Y.n = 1;
    pp = (int)E;
    {
        BP m = E >= 0 ? PX : PY;
        int k = E >= 0 ? (int)E : (int)-E;

        for (; k >= 6; k -= 6)
            mulp(m, 1, 6, 0);
        if (k)
            mulp(m, 1, k, 0);
    }

    la = bitlen(PX);
    lb = bitlen(PY);
    if (la > lb) {
        t = la - lb;
        shl(PY, t);
        pp += (int)t;
    } else if (lb > la) {
        t = lb - la;
        shl(PX, t);
        pp -= (int)t;
    }

    qw[0] = qw[1] = qw[2] = qw[3] = 0;
    for (i = 0; i < 56; i++) {
        qw[3] = (qw[3] << 1) | (qw[2] >> 15);
        qw[2] = (qw[2] << 1) | (qw[1] >> 15);
        qw[1] = (qw[1] << 1) | (qw[0] >> 15);
        qw[0] <<= 1;
        if (ge(PX, PY)) {
            sub(PX, PY);
            qw[0] |= 1;
        }
        shl1(PX);
    }
    low = X.n != 0;             /* the remainder: sticky */
    X.n = 0;

    bl = 0;
    for (i = 3; i >= 0; i--)
        if (qw[i]) {
            t = qw[i];
            bl = i * 16;
            while (t) {
                bl++;
                t >>= 1;
            }
            break;
        }
    e2 = (int)bl - 1 + pp - 55;
    if (e2 > 1023) {
        errno = ERANGE;
        return setbits(0x7FF0 | SIGN(neg), 0, 0, 0);
    }
    we = e2 - 52;
    if (we < -1074)
        we = -1074;
    r = (unsigned)(we - (pp - 55));
    if (r > 64)
        r = 64;
    guard = 0;
    while (r--) {
        low |= guard;
        guard = qw[0] & 1;
        qw[0] = (qw[0] >> 1) | (qw[1] << 15);
        qw[1] = (qw[1] >> 1) | (qw[2] << 15);
        qw[2] = (qw[2] >> 1) | (qw[3] << 15);
        qw[3] >>= 1;
    }
    inexact = guard | low;
    if (guard && (low || (qw[0] & 1)))
        if (++qw[0] == 0 && ++qw[1] == 0 && ++qw[2] == 0)
            ++qw[3];
    qw[3] += (u16)((we + 1074) << 4);
    if (qw[3] >= 0x7FF0) {
        errno = ERANGE;
        return setbits(0x7FF0 | SIGN(neg), 0, 0, 0);
    }
    if (inexact && qw[3] < 0x0010)
        errno = ERANGE;
    ret = setbits(qw[3] | SIGN(neg), qw[2], qw[1], qw[0]);
    return ret;
}
