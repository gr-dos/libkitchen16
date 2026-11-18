/* fastpath FILE|- [self]: the integral-value fast path (docs/spec/fmt.md,
 * "Exactness"). With FILE, the records of fast*.dat as fmt runs them: the
 * integral doubles below 2^53 through f, g, G and e at many precisions,
 * flags and widths, with expectations from the generator. With `self`, a
 * check that needs no data: integers n, made into doubles from their bits
 * (no floating-point operation), converted with %.0f, %f, %.3f, %g and the
 * rest, must read as n and the zeros the conversion adds, which is what
 * `%lu` gives for n. */

#define DRIVER_MEMCHECK
#include "lib/driver.h"

static unsigned dec(char *out, unsigned long n)
{
    char tmp[12];
    unsigned l = 0, o = 0;

    do {
        tmp[l++] = (char)('0' + (unsigned)(n % 10));
        n /= 10;
    } while (n);
    while (l)
        out[o++] = tmp[--l];
    out[o] = 0;
    return o;
}

/* the double n, as four words, without arithmetic on doubles */
static void from_ulong(unsigned long n, unsigned w[4])
{
    unsigned long frac_hi, frac_lo, lo, hi;
    unsigned p, k;

    w[0] = w[1] = w[2] = w[3] = 0;
    if (n == 0)
        return;
    for (p = 31; (n >> p) == 0; p--)
        ;
    /* the fraction: n without its top bit, 52 bits wide with it at the top */
    frac_lo = n & ((1UL << p) - 1UL);
    frac_hi = 0;
    for (k = 0; k < 52 - p; k++) {
        frac_hi = (frac_hi << 1) | (frac_lo >> 31);
        frac_lo <<= 1;
    }
    hi = ((unsigned long)(1023 + p) << 20) | frac_hi;
    lo = frac_lo;
    w[0] = (unsigned)(lo & 0xFFFFu);
    w[1] = (unsigned)(lo >> 16);
    w[2] = (unsigned)(hi & 0xFFFFu);
    w[3] = (unsigned)(hi >> 16);
}

static char want[80], got[120];

static void pad(char *s, unsigned width, int left, int zero)
{
    unsigned l = (unsigned)strlen(s), k;

    if (l >= width)
        return;
    k = width - l;
    if (left) {
        memset(s + l, ' ', k);
    } else {
        unsigned sign = (s[0] == '-' || s[0] == '+' || s[0] == ' ') ? 1 : 0;

        memmove(s + sign + k, s + sign, l - sign);
        memset(s + sign, zero ? '0' : ' ', k);
        if (!zero && sign) {    /* the sign goes next to the digits */
            s[k] = s[0];
            s[0] = ' ';
        }
    }
    s[width] = 0;
}

static void expect(unsigned long n, int neg, const char *sign_flag, const char *frac, unsigned width, int left, int zero)
{
    char digits[12];
    unsigned o = 0;

    dec(digits, n);
    if (neg)
        want[o++] = '-';
    else if (sign_flag[0])
        want[o++] = sign_flag[0];
    strcpy(want + o, digits);
    strcat(want, frac);
    pad(want, width, left, zero);
}

static void bad(unsigned long n, const char *fmt)
{
    if (shown < MAXSHOW) {
        fail_head("self");
        t_puts("n ");
        t_putu((unsigned)(n & 0xFFFFu));
        t_puts("+65536*");
        t_putu((unsigned)(n >> 16));
        t_puts(" fmt ");
        put_esc(fmt, (unsigned)strlen(fmt));
        t_puts(" got ");
        put_esc(got, (unsigned)strlen(got));
        t_puts(" want ");
        put_esc(want, (unsigned)strlen(want));
        t_puts("\r\n");
    }
    failures++;
}

static unsigned long seed = 12345UL;

static unsigned long next_value(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

/* one n through the conversions that must print it as an integer */
static void one(unsigned long n, int neg, unsigned which)
{
    unsigned w[4];
    unsigned ndig;
    char digits[12];
    int r;

    ndig = dec(digits, n);
    from_ulong(n, w);
    if (neg)
        w[3] |= 0x8000u;

#define TRY(fmt, sign, frac, width, left, zero) \
    expect(n, neg, sign, frac, width, left, zero); \
    r = (which & 1) ? call_sprintf(got, fmt, w, 4) : call_snprintf(got, sizeof got, fmt, w, 4); \
    if (r != (int)strlen(want) || strcmp(got, want) != 0) \
        bad(n, fmt)

    TRY("%.0f", "", "", 0, 0, 0);
    TRY("%f", "", ".000000", 0, 0, 0);
    TRY("%.3f", "", ".000", 0, 0, 0);
    TRY("%#.0f", "", ".", 0, 0, 0);
    TRY("%+.0f", "+", "", 0, 0, 0);
    TRY("% .0f", " ", "", 0, 0, 0);
    TRY("%14.0f", "", "", 14, 0, 0);
    TRY("%-14.0f", "", "", 14, 1, 0);
    TRY("%014.0f", "", "", 14, 0, 1);
    TRY("%+014.2f", "+", ".00", 14, 0, 1);
    TRY("%20.10f", "", ".0000000000", 20, 0, 0);
    TRY("%.20f", "", ".00000000000000000000", 0, 0, 0);
    if (ndig <= 10) {
        TRY("%.10g", "", "", 0, 0, 0);
        TRY("%.17g", "", "", 0, 0, 0);
        TRY("%.20G", "", "", 0, 0, 0);
    }
    if (ndig <= 6) {
        TRY("%g", "", "", 0, 0, 0);
    }
}

int main(int argc, char **argv)
{
    FILE *fp;
    struct rec r;
    int rc;
    unsigned long n;
    unsigned i, count = 0;

    CHECK(argc >= 1);
    if (argc < 2) {
        t_puts("usage: fastpath FILE|- [self]\r\n");
        return 2;
    }
    if (argv[1][0] != '-') {
        fp = fopen(argv[1], "rb");
        if (fp == 0) {
            t_puts("cannot open the data file\r\n");
            return 2;
        }
        while ((rc = next_rec(fp, &r)) > 0) {
            run_rec(&r, 0);
            count++;
        }
        fclose(fp);
        if (rc < 0 || count == 0) {
            fail_head("data");
            t_puts("bad or empty file\r\n");
        }
    }
    if (argc > 2 && argv[2][0] == 's') {
        unsigned which = 0;

        for (n = 0; n <= 130; n++)
            one(n, 0, which++);
        for (n = 131; n < 4294967295UL / 3; n = n * 3 + 1) {    /* 131, 394, ... */
            one(n, 0, which++);
            one(n, 1, which++);
        }
        for (i = 0; i < 32; i++) {
            n = 1UL << i;
            one(n, 0, which++);
            one(n - 1, 0, which++);
            one(n + 1, 1, which++);
        }
        for (n = 1; n <= 1000000000UL; n *= 10) {
            one(n, 0, which++);
            one(n - 1, 1, which++);
            one(n + 1, 0, which++);
        }
        one(4294967295UL, 0, which++);
        one(4294967295UL, 1, which++);
        one(2147483648UL, 0, which++);
        for (i = 0; i < 200; i++) {
            n = next_value();
            one(n >> (next_value() & 31), (int)(i & 1), which++);
        }
        count++;
    }
    if (failures) {
        t_putu(failures);
        t_puts(" failures\r\n");
    }
    return done();
}
