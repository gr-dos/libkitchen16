/* bench GROUP: the time of one call of each function of GROUP (fpc, fmt,
 * conv or math), on the machine tests/bench/run gives it: DOSBox-X, an 8086
 * at a fixed 315 cycles per millisecond, about an IBM PC's 4.77 MHz 8088
 * (docs/DESIGN.md, section 8, "Speed"). Not a test: it never fails, and is
 * run by `make bench`, not `make test`.
 *
 * Each operation is a function of no arguments that does one call on
 * volatile operands. It is called repeatedly from one BIOS timer tick (INT
 * 8, 18.2 Hz, read at 0040:006C) until SPAN ticks have gone, and the time
 * per call is the ticks over the calls, less the same figure for `none`,
 * which only copies its operand: what is left is the call itself, its
 * arguments and the helpers it uses. Prints one line per operation:
 *
 *   NAME TENTHS-OF-A-MICROSECOND CALLS
 *
 * The tick is the only clock. It is read after every four calls, so the
 * time is good to one tick in the SPAN or more ticks it covers: about 3%,
 * however few calls that was (a slow function makes only a few). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dos.h>
#include <libc16.h>
#include <kitchen16.h>
#include "testio.h"

#define SPAN 36u                /* ticks: about 2 seconds */
#define TICK_TENTHS_US 549254ul /* one tick, 1/18.2065 s, in tenths of a us */

static volatile double a = 1.2345678901234567, b = 7.6543210987654321, r;
static volatile double small_a = 0.5, big_a = 123456.789, deg = 1.0;
static volatile long li = 123456789L, lo;
static volatile unsigned long ul = 3000000000ul, ulo;
static volatile float fl = 1.5f, flo;
static volatile int ir;
static char buf[64];

static unsigned long ticks(void)
{
    return *(volatile unsigned long __far *)MK_FP(0x40, 0x6C);
}

typedef void (*op_t)(void);
struct op { const char *name; op_t f; };

/* the loop's own cost */
static void none(void)      { r = a; }

/* fpc: the helpers, through the operators the compiler turns into them */
static void fda(void)       { r = a + b; }
static void fds(void)       { r = a - b; }
static void fdm(void)       { r = a * b; }
static void fdd(void)       { r = a / b; }
static void fdc(void)       { ir = a < b; }
static void fdn(void)       { r = -a; }
static void fdi4(void)      { lo = (long)a; }
static void fdu4(void)      { ulo = (unsigned long)big_a; }
static void i4fd(void)      { r = (double)li; }
static void u4fd(void)      { r = (double)ul; }
static void fdfs(void)      { flo = (float)a; }
static void fsfd(void)      { r = (double)fl; }

/* fmt: snprintf, the shortest and the longest of Perl's formats among them */
static void sn_d(void)      { ir = snprintf(buf, sizeof buf, "%d", 12345); }
static void sn_g_int(void)  { ir = snprintf(buf, sizeof buf, "%.20g", 42.0); }
static void sn_g(void)      { ir = snprintf(buf, sizeof buf, "%g", a); }
static void sn_g20(void)    { ir = snprintf(buf, sizeof buf, "%.20g", a); }
static void sn_g20_tenth(void) { ir = snprintf(buf, sizeof buf, "%.20g", 0.1); }
static void sn_e(void)      { ir = snprintf(buf, sizeof buf, "%e", a); }
static void sn_f(void)      { ir = snprintf(buf, sizeof buf, "%.2f", big_a); }
static void sn_g_big(void)  { ir = snprintf(buf, sizeof buf, "%.17g", 1.7976931348623157e308); }
static void sn_g_tiny(void) { ir = snprintf(buf, sizeof buf, "%.17g", 4.9406564584124654e-324); }

/* conv */
static void st_int(void)    { r = strtod("12345", (char **)0); }
static void st_short(void)  { r = strtod("3.14159", (char **)0); }
static void st_17(void)     { r = strtod("1.2345678901234567", (char **)0); }
static void st_e300(void)   { r = strtod("1.7976931348623157e308", (char **)0); }
static void st_half(void)   { r = strtod("9007199254740993", (char **)0); }
static void at_short(void)  { r = atof("3.14159"); }

/* math */
static void m_sqrt(void)    { r = sqrt(b); }
static void m_floor(void)   { r = floor(b); }
static void m_fmod(void)    { r = fmod(b, a); }
static void m_modf(void)    { double ip; r = modf(b, &ip); }
static void m_sin(void)     { r = sin(deg); }
static void m_cos(void)     { r = cos(deg); }
static void m_tan(void)     { r = tan(deg); }
static void m_atan(void)    { r = atan(deg); }
static void m_atan2(void)   { r = atan2(a, b); }
static void m_exp(void)     { r = exp(a); }
static void m_log(void)     { r = log(b); }
static void m_log10(void)   { r = log10(b); }
static void m_pow_i(void)   { r = pow(b, 10.0); }
static void m_pow(void)     { r = pow(b, a); }
static void m_sinh(void)    { r = sinh(small_a); }
static void m_asin(void)    { r = asin(small_a); }

static const struct op fpc_ops[] = {
    { "__FDA a+b", fda }, { "__FDS a-b", fds }, { "__FDM a*b", fdm },
    { "__FDD a/b", fdd }, { "__FDC a<b", fdc }, { "__FDN -a", fdn },
    { "__FDI4 (long)a", fdi4 }, { "__FDU4 (unsigned long)a", fdu4 },
    { "__I4FD (double)long", i4fd }, { "__U4FD (double)unsigned long", u4fd },
    { "__FDFS (float)a", fdfs }, { "__FSFD (double)float", fsfd }, { 0, 0 }
};
static const struct op fmt_ops[] = {
    { "snprintf %d", sn_d }, { "snprintf %.20g 42", sn_g_int },
    { "snprintf %g", sn_g }, { "snprintf %.20g", sn_g20 },
    { "snprintf %.20g 0.1", sn_g20_tenth }, { "snprintf %e", sn_e },
    { "snprintf %.2f", sn_f }, { "snprintf %.17g DBL_MAX", sn_g_big },
    { "snprintf %.17g DBL_TRUE_MIN", sn_g_tiny }, { 0, 0 }
};
static const struct op conv_ops[] = {
    { "strtod 12345", st_int }, { "strtod 3.14159", st_short },
    { "strtod 17 digits", st_17 }, { "strtod DBL_MAX", st_e300 },
    { "strtod 2^53+1 (halfway)", st_half }, { "atof 3.14159", at_short }, { 0, 0 }
};
static const struct op math_ops[] = {
    { "sqrt", m_sqrt }, { "floor", m_floor }, { "fmod", m_fmod }, { "modf", m_modf },
    { "sin", m_sin }, { "cos", m_cos }, { "tan", m_tan }, { "atan", m_atan },
    { "atan2", m_atan2 }, { "exp", m_exp }, { "log", m_log }, { "log10", m_log10 },
    { "pow x^10", m_pow_i }, { "pow x^y", m_pow }, { "sinh", m_sinh },
    { "asin", m_asin }, { 0, 0 }
};

/* tenths of a microsecond per call of f, and the number of calls */
static unsigned long time_op(op_t f, unsigned long *calls)
{
    unsigned long t0, t, n = 0;

    t0 = ticks();
    while (ticks() == t0)
        ;
    t0 = ticks();
    do {
        f(); f(); f(); f();
        n += 4;
        t = ticks() - t0;
    } while (t < SPAN);
    *calls = n;
    return (t * TICK_TENTHS_US + n / 2) / n;
}

static void put_ul(unsigned long v)
{
    char s[12];

    t_puts(libc16_ultoa(v, s, 10));
}

int main(int argc, char **argv)
{
    const struct op *ops;
    unsigned long base, t, n;

    if (argc < 2)
        return 2;
    if (strcmp(argv[1], "fpc") == 0)
        ops = fpc_ops;
    else if (strcmp(argv[1], "fmt") == 0)
        ops = fmt_ops;
    else if (strcmp(argv[1], "conv") == 0)
        ops = conv_ops;
    else if (strcmp(argv[1], "math") == 0)
        ops = math_ops;
    else
        return 2;
    base = time_op(none, &n);
    t_puts("none ");
    put_ul(base);
    t_puts(" ");
    put_ul(n);
    t_puts("\r\n");
    for (; ops->name; ops++) {
        t = time_op(ops->f, &n);
        t_puts(ops->name);
        t_puts(" ");
        put_ul(t > base ? t - base : 0);
        t_puts(" ");
        put_ul(n);
        t_puts("\r\n");
    }
    return 0;
}
