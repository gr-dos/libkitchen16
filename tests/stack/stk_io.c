/* stk_io [-v]: the stack snprintf and fprintf use (docs/spec/fmt.md and
 * fmtio.md, "Memory and stack": at most 900 bytes for the formatting and
 * 160 more for a stream), on the formats and values that reach the widest
 * paths: %.1000f of the largest and the smallest values, %e and %g at high
 * precision, and every value of V. fprintf writes to the file NUL through
 * libc16's streams and DOS, so its figure includes libc16's fwrite and
 * write (and a few tens of bytes of interrupt, see lib/stack.h). */

#define WANT_VALUES
#include "lib/stack.h"

#define LIMIT_SN 900u
#define LIMIT_FP 960u

static char buf[1500];

static const char *const fmts[] = {
    "%.1000f", "%.60e", "%.40g", "%e", "%f", "%g", "%12.3f", "%-+#30.0e", "%.0f", "%#.0g"
};
#define NF (sizeof fmts / sizeof fmts[0])

static struct high sn[] = { { "snprintf", 0, 0 } };
static struct high fp[] = { { "fprintf", 0, 0 } };

int main(int argc, char **argv)
{
    FILE *f;
    unsigned i, j;
    int v = has_v(argc, argv);

    values();
    f = fopen("NUL", "wb");
    CHECK(f != 0);
    if (f == 0)
        return done();
    for (i = 0; i < NF; i++)
        for (j = 0; j < NV; j++) {
            note(sn, measure(K_SNPRINTF, (fn_t)snprintf, V[j], 0.0, (int)sizeof buf, buf, fmts[i]), i * NV + j);
            note(fp, measure(K_FPRINTF, (fn_t)fprintf, V[j], 0.0, 0, f, fmts[i]), i * NV + j);
        }
    fclose(f);
    report(sn, 1, LIMIT_SN, v);
    report(fp, 1, LIMIT_FP, v);
    return done();
}
