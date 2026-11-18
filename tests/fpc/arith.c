/* + - * / and unary - on doubles (docs/spec/fpc.md, "Arithmetic"), against the
 * corpus of tests/fpc/gen: usage  arith add|sub|mul|div|neg
 *
 * Each row is computed in four ways, so that every call-site shape is used:
 * both operands in global objects (g), the right operand in a local (l), both
 * through pointers (p: far in the compact and large models, so the E forms)
 * and the left operand in the object with the right through a pointer (m).
 * Operands are volatile, so nothing is folded, and are assembled from words
 * without floating-point operations. */

#include "lib/rows.h"

static volatile double va, vb, vr;
static volatile double *volatile pa = &va;
static volatile double *volatile pb = &vb;

static u16 aw[4], bw[4], rw[4];
static int rnan;

static void chk(char op, char form)
{
    u16 got[4];
    char tag[3];

    grab(got, (volatile u16 *)&vr, 4);
    if (!same_bits(got, rw, 4, rnan)) {
        tag[0] = op;
        tag[1] = form;
        tag[2] = '\0';
        fail_row(tag, got, 4);
    }
}

#define CLEAR() poke((volatile u16 *)&vr, zero, 4)

#define RUN(OP, C) \
    do { \
        volatile double lb; \
        CLEAR(); vr = va OP vb; chk(C, 'g'); \
        lb = vb; \
        CLEAR(); vr = va OP lb; chk(C, 'l'); \
        CLEAR(); vr = *pa OP *pb; chk(C, 'p'); \
        CLEAR(); vr = va OP *pb; chk(C, 'm'); \
    } while (0)

static const u16 zero[4] = { 0, 0, 0, 0 };

int main(int argc, char **argv)
{
    const char *file;
    int op, an, bn;

    if (argc < 2) {
        t_puts("usage: arith add|sub|mul|div|neg\r\n");
        return 2;
    }
    op = argv[1][0];
    file = op == 'a' ? "ADD.DAT" : op == 's' ? "SUB.DAT" : op == 'm' ? "MUL.DAT" : op == 'd' ? "DIV.DAT" : "NEG.DAT";
    if (!open_data(file))
        return 1;
    while (next_row()) {
        const char *p = row;

        p = get_field(p, aw, 4, &an);
        if (p && op == 'n') {
            u16 got[4];

            poke((volatile u16 *)&va, aw, 4);
            p = get_field(p, rw, 4, &rnan);
            if (p == NULL || an)
                bad_row();
            CLEAR(); vr = -va;
            grab(got, (volatile u16 *)&vr, 4);
            if (!same_bits(got, rw, 4, 0))
                fail_row("-g", got, 4);
            CLEAR(); vr = -*pa;
            grab(got, (volatile u16 *)&vr, 4);
            if (!same_bits(got, rw, 4, 0))
                fail_row("-p", got, 4);
            continue;
        }
        if (p)
            p = get_field(p, bw, 4, &bn);
        if (p)
            p = get_field(p, rw, 4, &rnan);
        if (p == NULL || an || bn) {
            bad_row();
            continue;
        }
        poke((volatile u16 *)&va, aw, 4);
        poke((volatile u16 *)&vb, bw, 4);
        switch (op) {
        case 'a': RUN(+, '+'); break;
        case 's': RUN(-, '-'); break;
        case 'm': RUN(*, '*'); break;
        default:  RUN(/, '/'); break;
        }
    }
    return done_report();
}
