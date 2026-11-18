/* stk_fpc [-v]: the stack each -fpc helper uses (docs/spec/fpc.md: at
 * most 64 bytes, and the helpers call nothing outside libkitchen16). Each
 * operation of the compiler is measured through a function that performs
 * it, on every pair of the values of V (the right operand reached through
 * a pointer, which is a far pointer in the compact and large models, so the
 * E forms are measured there), and the conversions on the values that
 * reach every branch. The row "(nothing)" is the measuring function with
 * no helper in it (its own frame and the call to it); its figure is taken
 * off every helper's, so the others are what the helper itself uses, its
 * return address included. */

#define WANT_VALUES
#include "lib/stack.h"

#define LIMIT 64u

static volatile double A, B, R;
static volatile double *pa = &A, *pb = &B, *pr = &R;
static volatile long L;
static volatile unsigned long UL;
static volatile float F;
static volatile int I;

static void op_nop(void) { *pr = *pa; }
static void op_add(void) { *pr = *pa + *pb; }
static void op_sub(void) { *pr = *pa - *pb; }
static void op_mul(void) { *pr = *pa * *pb; }
static void op_div(void) { *pr = *pa / *pb; }
static void op_neg(void) { *pr = -*pa; }
static void op_lt(void) { I = *pa < *pb; }
static void op_eq(void) { I = *pa == *pb; }
static void op_ge(void) { I = *pa >= *pb; }
static void op_tol(void) { L = (long)*pa; }
static void op_toul(void) { UL = (unsigned long)*pa; }
static void op_toi(void) { I = (int)*pa; }
static void op_froml(void) { *pr = (double)L; }
static void op_fromul(void) { *pr = (double)UL; }
static void op_tof(void) { F = (float)*pa; }
static void op_fromf(void) { *pr = (double)F; }

struct op { const char *name; void (*f)(void); int two; int conv; };

static struct op ops[] = {
    { "(nothing)", op_nop, 0, 0 },
    { "FDA", op_add, 1, 0 }, { "FDS", op_sub, 1, 0 }, { "FDM", op_mul, 1, 0 },
    { "FDD", op_div, 1, 0 }, { "FDN", op_neg, 0, 0 }, { "FDC<", op_lt, 1, 0 },
    { "FDC==", op_eq, 1, 0 }, { "FDC>=", op_ge, 1, 0 }, { "FDI4", op_tol, 0, 0 },
    { "FDU4", op_toul, 0, 0 }, { "FDI4(int)", op_toi, 0, 0 }, { "I4FD", op_froml, 0, 1 },
    { "U4FD", op_fromul, 0, 2 }, { "FDFS", op_tof, 0, 0 }, { "FSFD", op_fromf, 0, 3 }
};
#define NOPS (sizeof ops / sizeof ops[0])

int main(int argc, char **argv)
{
    static const long ls[] = { 0L, 1L, -1L, 32767L, 32768L, 65535L, 65536L, 0x7FFFFFFFL, -0x7FFFFFFFL - 1, 0x12345678L };
    static const unsigned long us[] = { 0UL, 1UL, 65535UL, 65536UL, 0x7FFFFFFFUL, 0x80000000UL, 0xFFFFFFFFUL, 0x12345678UL };
    static const unsigned fs[][2] = {
        { 0x0000, 0x0000 }, { 0x3F80, 0x0000 }, { 0x0000, 0x0001 }, { 0x007F, 0xFFFF }, { 0x0080, 0x0000 },
        { 0x7F7F, 0xFFFF }, { 0x7F80, 0x0000 }, { 0x7FC0, 0x0000 }, { 0xFF80, 0x0000 }, { 0x8000, 0x0001 }
    };
    static struct high h[NOPS];
    unsigned i, j, k;
    int v = has_v(argc, argv);

    values();
    for (i = 0; i < NOPS; i++) {
        h[i].name = ops[i].name;
        if (ops[i].conv == 1) {
            for (j = 0; j < sizeof ls / sizeof ls[0]; j++) {
                L = ls[j];
                note(&h[i], measure(K_VOID, (fn_t)ops[i].f, 0.0, 0.0, 0, 0, 0), j);
            }
        } else if (ops[i].conv == 2) {
            for (j = 0; j < sizeof us / sizeof us[0]; j++) {
                UL = us[j];
                note(&h[i], measure(K_VOID, (fn_t)ops[i].f, 0.0, 0.0, 0, 0, 0), j);
            }
        } else if (ops[i].conv == 3) {
            for (j = 0; j < sizeof fs / sizeof fs[0]; j++) {
                ((volatile unsigned *)&F)[1] = fs[j][0];
                ((volatile unsigned *)&F)[0] = fs[j][1];
                note(&h[i], measure(K_VOID, (fn_t)ops[i].f, 0.0, 0.0, 0, 0, 0), j);
            }
        } else {
            for (j = 0; j < NV; j++)
                for (k = 0; k < (ops[i].two ? NV : 1); k++) {
                    A = V[j];
                    B = V[k];
                    note(&h[i], measure(K_VOID, (fn_t)ops[i].f, 0.0, 0.0, 0, 0, 0), j * NV + k);
                }
        }
    }
    for (i = 1; i < NOPS; i++)
        h[i].used = h[i].used > h[0].used ? h[i].used - h[0].used : 0;
    report(h, NOPS, LIMIT, v);
    return done();
}
