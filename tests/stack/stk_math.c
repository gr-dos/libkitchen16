/* stk_math [-v]: the stack each <math.h> function uses (docs/spec/math.md,
 * "Size and linking"), with the real fpc helpers linked: every value of V
 * for every function of one argument, every pair of a smaller set for those
 * of two, hard cases of pow and ldexp, so the error branches, the special
 * values, the widest argument reduction and the nested helper calls are all
 * in the measurement. Fails if a function uses more than LIMIT bytes
 * (including its arguments and return address); -v prints each function's
 * maximum. */

#include <math.h>
#define WANT_VALUES
#include "lib/stack.h"

#define LIMIT 448u

#define F1(f) { #f, 0, 0 }
static struct high one[] = {
    F1(sin), F1(cos), F1(tan), F1(asin), F1(acos), F1(atan), F1(sinh), F1(cosh),
    F1(tanh), F1(exp), F1(log), F1(log10), F1(sqrt), F1(fabs), F1(floor), F1(ceil)
};
static fn_t one_f[] = {
    (fn_t)sin, (fn_t)cos, (fn_t)tan, (fn_t)asin, (fn_t)acos, (fn_t)atan, (fn_t)sinh,
    (fn_t)cosh, (fn_t)tanh, (fn_t)exp, (fn_t)log, (fn_t)log10, (fn_t)sqrt, (fn_t)fabs,
    (fn_t)floor, (fn_t)ceil
};
#define N1 (sizeof one / sizeof one[0])

static struct high two[] = { F1(atan2), F1(pow), F1(fmod) };
static fn_t two_f[] = { (fn_t)atan2, (fn_t)pow, (fn_t)fmod };
#define N2 (sizeof two / sizeof two[0])

static struct high other[] = { F1(ldexp), F1(frexp), F1(modf) };

/* extra operand pairs for pow, which has several paths */
static const double pw[][2] = {
    { 2.0, 10.0 }, { 2.0, 0.5 }, { 10.0, -5.0 }, { 2.5, 100.5 }, { 7.0, 1023.0 },
    { 3.0, -700.0 }, { 1.0005, 2000.0 }, { 0.9999, 5000.0 }, { 1.0000000001, 1e9 },
    { 10.0, 308.0 }, { 10.0, -323.0 }, { 1e300, 2.0 }, { 2.0, -1074.0 },
    { 2.0, 1023.0 }, { 1e-300, 1e-300 }, { 1.5, 1025.0 }, { 0.5, 1030.0 },
    { 123456.789, 3.7 }, { 1e-5, -50.25 }, { 9.99, 1.0 / 3.0 }
};

int main(int argc, char **argv)
{
    static const int ns[] = { -32768, -2000, -1074, -1, 0, 1, 1023, 2000, 32767 };
    unsigned i, j, k, idx;
    int e;
    double ip;

    values();
    for (i = 0; i < N1; i++)
        for (j = 0; j < NV; j++)
            note(&one[i], measure(K_D_D, one_f[i], V[j], 0.0, 0, 0, 0), j);
    for (i = 0; i < N2; i++) {
        idx = 0;
        for (j = 0; j < NV; j += 2)
            for (k = 0; k < NV; k += 2)
                note(&two[i], measure(K_D_DD, two_f[i], V[j], V[k], 0, 0, 0), idx++);
        for (j = 0; j < NV; j++)
            for (k = 1; k < NV; k += 2)
                note(&two[i], measure(K_D_DD, two_f[i], V[j], V[k], 0, 0, 0), idx++);
    }
    for (j = 0; j < sizeof pw / sizeof pw[0]; j++)
        note(&two[1], measure(K_D_DD, (fn_t)pow, pw[j][0], pw[j][1], 0, 0, 0), 1000 + j);
    for (j = 0; j < NV; j++) {
        for (k = 0; k < sizeof ns / sizeof ns[0]; k++)
            note(&other[0], measure(K_D_DI, (fn_t)ldexp, V[j], 0.0, ns[k], 0, 0), j * 16 + k);
        note(&other[1], measure(K_D_DPI, (fn_t)frexp, V[j], 0.0, 0, &e, 0), j);
        note(&other[2], measure(K_D_DPD, (fn_t)modf, V[j], 0.0, 0, &ip, 0), j);
    }
    {
        int v = has_v(argc, argv);

        report(one, N1, LIMIT, v);
        report(two, N2, LIMIT, v);
        report(other, 3, LIMIT, v);
    }
    return done();
}
