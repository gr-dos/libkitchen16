/* nancmp: what the comparison operators do with the flags of __FDC and
 * __EDC (docs/spec/fpc.md, "Comparison"; issue 2 of libkitchen16), as the
 * compiler's own call sites show it.
 *
 *  1. Every operator, as a value, as a branch and through pointers (the E
 *     forms in the compact and large models), on every ordered pair and on
 *     every pair with a NaN: ordered pairs give the IEEE 754 result; an
 *     unordered pair is "greater", so != > >= are true and == < <= false.
 *     No flags result can do better (docs/spec/fpc.md shows why).
 *  2. A comparison with a literal 0.0, !x, if (x), x && y and x ? a : b are
 *     compiled without a helper, as a test of the high 16 bits of the
 *     operand: -0.0 is not zero and is less than zero, a subnormal with
 *     those bits clear is zero, and a NaN is non-zero. This is the
 *     compiler's, not the helpers', so the test records it.
 *  3. The idioms the spec gives for a program that needs IEEE results (a
 *     zero that is not a literal, an explicit NaN test) do give them.
 *
 * The operands are made from words, with no floating-point operation. */

#include "lib/report.h"

typedef unsigned short u16;

#define NV 20
static double V[NV];
static u16 W[NV][4];

static const u16 words[NV][4] = {
    { 0x0000, 0, 0, 0 },          /* +0 */
    { 0x8000, 0, 0, 0 },          /* -0 */
    { 0x3FF0, 0, 0, 0 },          /* 1 */
    { 0xBFF0, 0, 0, 0 },          /* -1 */
    { 0x4000, 0, 0, 0 },          /* 2 */
    { 0x0000, 0, 0, 1 },          /* the smallest subnormal */
    { 0x8000, 0, 0, 1 },          /* its negative */
    { 0x000F, 0xFFFF, 0xFFFF, 0xFFFF },   /* the largest subnormal */
    { 0x0010, 0, 0, 0 },          /* the smallest normal */
    { 0x7FEF, 0xFFFF, 0xFFFF, 0xFFFF },   /* DBL_MAX */
    { 0xFFEF, 0xFFFF, 0xFFFF, 0xFFFF },   /* -DBL_MAX */
    { 0x7FF0, 0, 0, 0 },          /* +inf */
    { 0xFFF0, 0, 0, 0 },          /* -inf */
    { 0x7FF8, 0, 0, 0 },          /* a quiet NaN */
    { 0xFFF8, 0, 0, 0 },          /* the same with the sign bit */
    { 0x7FF4, 0, 0, 1 },          /* a signalling NaN */
    { 0x7FF8, 0x1234, 0, 0xFFFF },        /* a NaN with a payload */
    { 0x3FF0, 0, 0, 1 },          /* 1 + 2^-52 */
    { 0x0001, 0, 0, 0 },          /* a normal with high bits set, small */
    { 0x0000, 0xFFFF, 0, 0 }      /* a subnormal whose high 16 bits are clear */
};

/* a reference comparison on the bits: -1, 0, 1, or 2 if unordered */
static int rel(const u16 *a, const u16 *b)
{
    int an = (a[3] & 0x7FF0) == 0x7FF0 && ((a[3] & 0x000F) | a[2] | a[1] | a[0]);
    int bn = (b[3] & 0x7FF0) == 0x7FF0 && ((b[3] & 0x000F) | b[2] | b[1] | b[0]);
    int az = !((a[3] & 0x7FFF) | a[2] | a[1] | a[0]);
    int bz = !((b[3] & 0x7FFF) | b[2] | b[1] | b[0]);
    int as = (a[3] & 0x8000) != 0, bs = (b[3] & 0x8000) != 0;
    int i, c;

    if (an || bn)
        return 2;
    if (az && bz)
        return 0;
    if (as != bs)
        return as ? -1 : 1;
    c = 0;
    for (i = 3; i >= 0 && c == 0; i--) {
        u16 x = i == 3 ? (a[3] & 0x7FFF) : a[i];
        u16 y = i == 3 ? (b[3] & 0x7FFF) : b[i];

        c = x < y ? -1 : x > y ? 1 : 0;
    }
    return as ? -c : c;
}

/* the six operators, as the compiler's flags give them: an unordered pair is "greater" */
static int want(int op, int r)
{
    switch (op) {
    case 0: return r == 0;
    case 1: return r != 0;
    case 2: return r == -1;
    case 3: return r == -1 || r == 0;
    case 4: return r == 1 || r == 2;
    default: return r != -1;
    }
}

#define OPS(X) X(0, ==) X(1, !=) X(2, <) X(3, <=) X(4, >) X(5, >=)
#define DEF(n, o) \
    static int val##n(double a, double b) { return a o b; } \
    static int br##n(double a, double b) { if (a o b) return 1; return 0; } \
    static int pt##n(double *a, double *b) { return *a o *b; }
OPS(DEF)
#define VAL(n, o) val##n,
#define BR(n, o) br##n,
#define PT(n, o) pt##n,
static int (*const vals[6])(double, double) = { OPS(VAL) };
static int (*const brs[6])(double, double) = { OPS(BR) };
static int (*const pts[6])(double *, double *) = { OPS(PT) };
static const char *const opname[6] = { "==", "!=", "<", "<=", ">", ">=" };

static void pairs(void)
{
    int i, j, op, r;

    for (i = 0; i < NV; i++)
        for (j = 0; j < NV; j++) {
            r = rel(W[i], W[j]);
            for (op = 0; op < 6; op++) {
                int w = want(op, r);
                int g1 = vals[op](V[i], V[j]) != 0;
                int g2 = brs[op](V[i], V[j]) != 0;
                int g3 = pts[op](&V[i], &V[j]) != 0;

                nchecks += 3;
                if (g1 != w && begin_fail("value")) { t_puts(opname[op]); t_puts(" "); t_putu(i); t_puts(" "); t_putu(j); t_puts("\r\n"); }
                if (g2 != w && begin_fail("branch")) { t_puts(opname[op]); t_puts(" "); t_putu(i); t_puts(" "); t_putu(j); t_puts("\r\n"); }
                if (g3 != w && begin_fail("pointer")) { t_puts(opname[op]); t_puts(" "); t_putu(i); t_puts(" "); t_putu(j); t_puts("\r\n"); }
            }
        }
}

/* 2: literal zero and truthiness: the high 16 bits only */
static int z_eq(double a) { return a == 0.0; }
static int z_ne(double a) { return a != 0.0; }
static int z_lt(double a) { return a < 0.0; }
static int z_le(double a) { return a <= 0.0; }
static int z_gt(double a) { return a > 0.0; }
static int z_ge(double a) { return a >= 0.0; }
static int z_not(double a) { return !a; }
static int z_if(double a) { if (a) return 1; return 0; }
static int z_cond(double a) { return a ? 1 : 0; }
static int z_and(double a, double b) { return a && b; }
static int p_eq(double *a) { return *a == 0.0; }
static int p_ne(double *a) { return *a != 0.0; }
static int p_if(double *a) { return *a ? 1 : 0; }

static const char *const lname[12] = {
    "==0.0", "!=0.0", "<0.0", "<=0.0", ">0.0", ">=0.0", "!x", "if(x)", "x?:", "*p==0.0", "*p!=0.0", "*p?:"
};

static void literal(void)
{
    int i, j, k;

    for (i = 0; i < NV; i++) {
        int w = (int)(short)W[i][3];     /* the high 16 bits, as a signed number */
        int z = W[i][3] == 0;
        int got[12], exp[12];

        got[0] = z_eq(V[i]) != 0;   exp[0] = z;
        got[1] = z_ne(V[i]) != 0;   exp[1] = !z;
        got[2] = z_lt(V[i]) != 0;   exp[2] = w < 0;
        got[3] = z_le(V[i]) != 0;   exp[3] = w <= 0;
        got[4] = z_gt(V[i]) != 0;   exp[4] = w > 0;
        got[5] = z_ge(V[i]) != 0;   exp[5] = w >= 0;
        got[6] = z_not(V[i]) != 0;  exp[6] = z;
        got[7] = z_if(V[i]) != 0;   exp[7] = !z;
        got[8] = z_cond(V[i]) != 0; exp[8] = !z;
        got[9] = p_eq(&V[i]) != 0;  exp[9] = z;
        got[10] = p_ne(&V[i]) != 0; exp[10] = !z;
        got[11] = p_if(&V[i]) != 0; exp[11] = !z;
        for (k = 0; k < 12; k++) {
            nchecks++;
            if (got[k] != exp[k] && begin_fail("literal")) {
                t_puts(lname[k]); t_puts(" "); t_putu(i); t_puts("\r\n");
            }
        }
        for (j = 0; j < NV; j++) {
            int both = W[i][3] != 0 && W[j][3] != 0;

            nchecks++;
            if ((z_and(V[i], V[j]) != 0) != both && begin_fail("and")) {
                t_puts("&& "); t_putu(i); t_puts(" "); t_putu(j); t_puts("\r\n");
            }
        }
    }
}

/* 3: the idioms of the spec */
static volatile double zero;

static int isnan_(double x)
{
    u16 *w = (u16 *)&x;

    return (w[3] & 0x7FF0) == 0x7FF0 && ((w[3] & 0x000F) | w[2] | w[1] | w[0]);
}

static void idioms(void)
{
    int i, j;

    for (i = 0; i < NV; i++) {
        int r = rel(W[i], W[0]);        /* against +0 */
        int nz = r != 0;                 /* x != 0, true for a NaN */

        nchecks += 4;
        if ((V[i] == zero) != (r == 0) && begin_fail("zero==")) { t_putu(i); t_puts("\r\n"); }
        if ((V[i] != zero) != nz && begin_fail("zero!=")) { t_putu(i); t_puts("\r\n"); }
        if ((V[i] < zero) != (r == -1) && begin_fail("zero<")) { t_putu(i); t_puts("\r\n"); }
        if ((isnan_(V[i]) != 0) != (r == 2) && begin_fail("isnan")) { t_putu(i); t_puts("\r\n"); }
        for (j = 0; j < NV; j++) {
            int q = rel(W[i], W[j]);
            int ordered = !isnan_(V[i]) && !isnan_(V[j]);

            nchecks += 2;
            if ((ordered && V[i] < V[j]) != (q == -1) && begin_fail("ieee<")) { t_putu(i); t_puts(" "); t_putu(j); t_puts("\r\n"); }
            if ((ordered && V[i] >= V[j]) != (q == 0 || q == 1) && begin_fail("ieee>=")) { t_putu(i); t_puts(" "); t_putu(j); t_puts("\r\n"); }
        }
    }
}

int main(void)
{
    int i;

    for (i = 0; i < NV; i++) {
        u16 *w = (u16 *)&V[i];

        w[3] = words[i][0]; w[2] = words[i][1]; w[1] = words[i][2]; w[0] = words[i][3];
        W[i][3] = words[i][0]; W[i][2] = words[i][1]; W[i][1] = words[i][2]; W[i][0] = words[i][3];
    }
    pairs();
    literal();
    idioms();
    return done_report();
}
