/* Printing with sprintf("%.17g") and reading back with strtod gives the same
 * bits, for a fixed pseudo-random set of finite doubles (all exponents, the
 * subnormals, and some special values). The doubles and the results are
 * handled as words, so that the program does no floating-point operation
 * of its own. */

#include <errno.h>
#include <kitchen16.h>
#include "testio.h"
#include "lib/hex.h"

#define COUNT 1200u

union dbl {
    double d;
    unsigned short w[4];
};

static unsigned rs = 0xACE1u;

/* xorshift of 16 bits (7, 9, 8): period 65535 */
static unsigned rnd(void)
{
    rs ^= rs << 7;
    rs ^= rs >> 9;
    rs ^= rs << 8;
    return rs;
}

static unsigned reported;
static unsigned failures;

static void one(union dbl *in)
{
    union dbl out;
    char buf[48];
    char *end;
    int n;
    int i;

    n = sprintf(buf, "%.17g", in->d);
    errno = 0;
    out.d = strtod(buf, &end);
    if (n < 1 || end != buf + n || out.w[0] != in->w[0] || out.w[1] != in->w[1] || out.w[2] != in->w[2]
        || out.w[3] != in->w[3]) {
        failures++;
        if (reported++ < 20) {
            t_puts("not ok ");
            for (i = 3; i >= 0; i--)
                t_puthex16(in->w[i]);
            t_puts(" printed ");
            t_puts(buf);
            t_puts(" read ");
            for (i = 3; i >= 0; i--)
                t_puthex16(out.w[i]);
            t_puts("\r\n");
        }
    }
}

int main(void)
{
    union dbl v;
    unsigned i;
    unsigned x;
    static const unsigned special[][4] = {
        { 0, 0, 0, 0x0000 }, { 0, 0, 0, 0x8000 }, { 1, 0, 0, 0 }, { 1, 0, 0, 0x8000 },
        { 0xFFFF, 0xFFFF, 0xFFFF, 0x000F }, { 0, 0, 0, 0x0010 }, { 0xFFFF, 0xFFFF, 0xFFFF, 0x7FEF },
        { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFEF }, { 0, 0, 0, 0x3FF0 }, { 0x9999, 0x9999, 0x9999, 0x3FB9 },
        { 0, 0, 0, 0x4340 }, { 1, 0, 0, 0x4340 }, { 0, 0, 0, 0x3CA0 }, { 0x6666, 0x6666, 0x6666, 0x3FE6 },
    };

    for (i = 0; i < sizeof special / sizeof special[0]; i++) {
        v.w[0] = special[i][0];
        v.w[1] = special[i][1];
        v.w[2] = special[i][2];
        v.w[3] = special[i][3];
        one(&v);
    }
    for (i = 0; i < COUNT; i++) {
        v.w[0] = rnd();
        v.w[1] = rnd();
        v.w[2] = rnd();
        v.w[3] = rnd();
        switch (i % 4) {
        case 0:                         /* any exponent */
            break;
        case 1:                         /* subnormal */
            v.w[3] &= 0x800F;
            break;
        case 2:                         /* every normal exponent in turn */
            x = 1 + (i / 4u * 7u) % 2046u;
            v.w[3] = (v.w[3] & 0x800F) | (x << 4);
            break;
        default:                        /* short mantissas: few bits set */
            v.w[0] = v.w[1] = 0;
            v.w[2] &= 0xFF00;
            break;
        }
        if ((v.w[3] & 0x7FF0) == 0x7FF0)
            v.w[3] &= 0xBFFF;
        one(&v);
    }
    if (failures == 0)
        t_puts("ok\r\n");
    return failures != 0;
}
