/* Strings of thousands of digits, built here at run time, convert exactly and
 * in bounded memory: this program runs with a small stack (rules.mk), and
 * the library has no buffer that grows with the input. The expected values
 * are exact (a tie, a value just above or below one, zero runs) and are
 * given as bits, so the program does no floating-point operation. */

#include <errno.h>
#include <kitchen16.h>
#include "testio.h"

#define N 5000

static char buf[N + N + 64];

static unsigned len;
static unsigned failures;

static void reset(void)
{
    len = 0;
}

static void put(const char *s)
{
    while (*s)
        buf[len++] = *s++;
}

static void fill(char c, unsigned n)
{
    while (n--)
        buf[len++] = c;
}

static void finish(void)
{
    buf[len] = '\0';
}

/* Convert buf: the result must be W3:0:0:W0 ... given as four words, the
 * endptr must be at END, and errno must be ERR (0 or ERANGE). */
static void conv(unsigned line, unsigned w3, unsigned w2, unsigned w1, unsigned w0, unsigned end, int erange)
{
    union {
        double d;
        unsigned short w[4];
    } u;
    char *e;

    finish();
    errno = 0;
    u.d = strtod(buf, &e);
    if (u.w[3] != w3 || u.w[2] != w2 || u.w[1] != w1 || u.w[0] != w0) {
        t_puts("bits, line ");
        t_putu(line);
        t_puts("\r\n");
        failures++;
    }
    if ((unsigned)(e - buf) != end) {
        t_puts("endptr, line ");
        t_putu(line);
        t_puts("\r\n");
        failures++;
    }
    if (errno != (erange ? ERANGE : 0)) {
        t_puts("errno, line ");
        t_putu(line);
        t_puts("\r\n");
        failures++;
    }
}

int main(void)
{
    /* 1 followed by 5000 zeros, and the exponent that cancels them: exactly 1 */
    reset(); put("1"); fill('0', N); put("e-5000");
    conv(__LINE__, 0x3FF0, 0, 0, 0, len, 0);

    /* 0.000...0001 with the exponent that cancels the zeros: exactly 1 */
    reset(); put("0."); fill('0', N); put("1e5001");
    conv(__LINE__, 0x3FF0, 0, 0, 0, len, 0);

    /* 2**53 + 1 followed by zeros is the tie, to even: 2**53 */
    reset(); put("9007199254740993"); fill('0', N); put("e-5000");
    conv(__LINE__, 0x4340, 0, 0, 0, len, 0);

    /* with a 1 as the last of the 5000 digits it is above the tie: 2**53 + 2 */
    reset(); put("9007199254740993"); fill('0', N - 1); put("1"); put("e-5000");
    conv(__LINE__, 0x4340, 0, 0, 1, len, 0);

    /* 2**53 + 3 is also a tie, to even: 2**53 + 4; just below it is 2**53 + 2 */
    reset(); put("9007199254740995"); fill('0', N); put("e-5000");
    conv(__LINE__, 0x4340, 0, 0, 2, len, 0);
    reset(); put("9007199254740994"); fill('9', N); put("e-5000");
    conv(__LINE__, 0x4340, 0, 0, 1, len, 0);

    /* N nines and the exponent that makes the value just below 10: rounds to 10 */
    reset(); fill('9', N); put("e-"); put("4999");
    conv(__LINE__, 0x4024, 0, 0, 0, len, 0);

    /* 1.1111... with 6000 digits: 10/9 */
    reset(); put("1."); fill('1', N + 1000);
    conv(__LINE__, 0x3FF1, 0xC71C, 0x71C7, 0x1C72, len, 0);

    /* too large and too small, by the length alone */
    reset(); put("1"); fill('0', N);
    conv(__LINE__, 0x7FF0, 0, 0, 0, len, 1);
    reset(); put("-0."); fill('0', N); put("1");
    conv(__LINE__, 0x8000, 0, 0, 0, len, 1);

    /* leading zeros, leading white space, and a long tail that is not part of the number */
    reset(); fill('0', N); put("1.5");
    conv(__LINE__, 0x3FF8, 0, 0, 0, len, 0);
    reset(); fill(' ', N); put("2");
    conv(__LINE__, 0x4000, 0, 0, 0, len, 0);
    reset(); put("1."); fill('0', N); put("x1");
    conv(__LINE__, 0x3FF0, 0, 0, 0, N + 2, 0);
    reset(); put("1e5x");
    conv(__LINE__, 0x40F8, 0x6A00, 0, 0, 3, 0);

    /* exact zero with a long run of digits and a huge exponent: no error */
    reset(); fill('0', N); put("e99999");
    conv(__LINE__, 0, 0, 0, 0, len, 0);

    if (failures == 0)
        t_puts("ok\r\n");
    return failures != 0;
}
