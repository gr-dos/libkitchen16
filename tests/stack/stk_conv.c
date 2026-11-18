/* stk_conv [-v]: the stack strtod uses (docs/spec/conv.md, "Size and
 * stack"), on strings that reach every path: the shortest and the 800-digit
 * and longer inputs, hard-to-round halfway cases, subnormal results, overflow
 * and underflow, exponents of any size, infinities and NaNs. Fails if
 * strtod uses more than LIMIT bytes; -v prints the maximum. atof is
 * strtod's one call and measured with it. */

#include "lib/stack.h"

#define LIMIT 160u

static char longbuf[1400];

static const char *const strs[] = {
    "0", "1", "-1.5", "  +2.5e3x", ".5", "5.", "1e22", "1e23", "123456789.123456789e-5",
    "1.7976931348623157e308", "1.7976931348623159e308", "1e309", "-1e999999999999",
    "4.9406564584124654e-324", "2.4703282292062327e-324", "2.4703282292062328e-324",
    "1e-324", "1e-400", "2.2250738585072011e-308", "2.2250738585072014e-308",
    "9007199254740993", "9007199254740992.99999999999999999999",
    "inf", "-Infinity", "nan", "NaN(abc)", "infinit", "nan(", "e5", "+", "0x1p3",
    "8.98846567431158e307", "1.00000000000000011102230246251565404236316680908203125",
    "1.00000000000000011102230246251565404236316680908203126",
    "0.00000000000000000000000000000000000000000000000000000000000000000000001",
    "1e-5000000000", "1e5000000000", "00000000000000000000000000000000000001e+0001"
};
#define NS (sizeof strs / sizeof strs[0])

static struct high h[] = { { "strtod", 0, 0 }, { "atof", 0, 0 } };

static void fill(char *p, unsigned n, char c)
{
    while (n--)
        *p++ = c;
}

int main(int argc, char **argv)
{
    unsigned i;
    int v = has_v(argc, argv);

    for (i = 0; i < NS; i++) {
        note(&h[0], measure(K_STR, (fn_t)strtod, 0.0, 0.0, 0, (void *)strs[i], 0), i);
        note(&h[1], measure(K_STR, (fn_t)atof, 0.0, 0.0, 0, (void *)strs[i], 0), i);
    }
    /* long inputs: the digits of a halfway case, then more digits, and a huge exponent */
    strcpy(longbuf, "1.00000000000000011102230246251565404236316680908203125");
    fill(longbuf + strlen(longbuf), 1300, '0');
    strcpy(longbuf + strlen(longbuf), "1");
    note(&h[0], measure(K_STR, (fn_t)strtod, 0.0, 0.0, 0, longbuf, 0), 100);
    fill(longbuf, 1390, '9');
    longbuf[1390] = 0;
    note(&h[0], measure(K_STR, (fn_t)strtod, 0.0, 0.0, 0, longbuf, 0), 101);
    strcpy(longbuf + 1000, "e-1000");
    note(&h[0], measure(K_STR, (fn_t)strtod, 0.0, 0.0, 0, longbuf, 0), 102);
    fill(longbuf, 1300, '0');
    strcpy(longbuf + 1300, "4.9406564584124654e-324");
    note(&h[0], measure(K_STR, (fn_t)strtod, 0.0, 0.0, 0, longbuf, 0), 103);
    report(h, 2, LIMIT, v);
    return done();
}
