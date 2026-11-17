/* double to float and float to double (docs/spec/fpc.md, "float"): usage
 * tofloat d2f   against D2F.DAT, rows "A F", F = (float)A
 * tofloat f2d   against F2D.DAT, rows "F D", D = (double)F
 * A NaN result is written N in the data: any NaN passes. */

#include "lib/rows.h"

static volatile double vd;
static volatile float vf;
static volatile double *volatile pd = &vd;
static volatile float *volatile pf = &vf;
static volatile double vr;
static volatile float vq;

static u16 dw[4], fw[2], rw[4];
static int rnan;

int main(int argc, char **argv)
{
    int d2f, an;

    if (argc < 2) {
        t_puts("usage: tofloat d2f|f2d\r\n");
        return 2;
    }
    d2f = argv[1][0] == 'd';
    if (!open_data(d2f ? "D2F.DAT" : "F2D.DAT"))
        return 1;
    while (next_row()) {
        const char *p;
        u16 got[4];

        if (d2f) {
            p = get_field(row, dw, 4, &an);
            if (p)
                p = get_field(p, rw, 2, &rnan);
            if (p == NULL || an) {
                bad_row();
                continue;
            }
            poke((volatile u16 *)&vd, dw, 4);
            vq = 1.5f;
            vq = (float)vd;
            grab(got, (volatile u16 *)&vq, 2);
            if (!same_bits(got, rw, 2, rnan))
                fail_row("d2f", got, 2);
            vq = 1.5f;
            vq = (float)*pd;
            grab(got, (volatile u16 *)&vq, 2);
            if (!same_bits(got, rw, 2, rnan))
                fail_row("d2f.p", got, 2);
        } else {
            p = get_field(row, fw, 2, &an);
            if (p)
                p = get_field(p, rw, 4, &rnan);
            if (p == NULL || an) {
                bad_row();
                continue;
            }
            poke((volatile u16 *)&vf, fw, 2);
            vr = 1.5;
            vr = (double)vf;
            grab(got, (volatile u16 *)&vr, 4);
            if (!same_bits(got, rw, 4, rnan))
                fail_row("f2d", got, 4);
            vr = 1.5;
            vr = (double)*pf;
            grab(got, (volatile u16 *)&vr, 4);
            if (!same_bits(got, rw, 4, rnan))
                fail_row("f2d.p", got, 4);
        }
    }
    return done_report();
}
