/* integer to double: (double) of a long, an unsigned long, an int and an
 * unsigned (docs/spec/fpc.md, "Conversions from integers"), against
 * FROMINT.DAT: each row is "L S U I W", L a 32-bit pattern and S, U, I, W the
 * doubles of (long)L, (unsigned long)L, the int and the unsigned made of
 * L's low word. */

#include "lib/rows.h"

static volatile long vl;
static volatile unsigned long vu;
static volatile int vi;
static volatile unsigned vw;
static volatile double vr;

static u16 lw[2], sw[4], uw[4], iw[4], ww[4];

static void chk(const char *tag, const u16 *want)
{
    u16 got[4];

    grab(got, (volatile u16 *)&vr, 4);
    if (!same_bits(got, want, 4, 0))
        fail_row(tag, got, 4);
}

int main(void)
{
    int n;

    if (!open_data("FROMINT.DAT"))
        return 1;
    while (next_row()) {
        const char *p = get_field(row, lw, 2, &n);
        int bad = n;

        if (p)
            p = get_field(p, sw, 4, &n);
        bad |= n;
        if (p)
            p = get_field(p, uw, 4, &n);
        bad |= n;
        if (p)
            p = get_field(p, iw, 4, &n);
        bad |= n;
        if (p)
            p = get_field(p, ww, 4, &n);
        bad |= n;
        if (p == NULL || bad) {
            bad_row();
            continue;
        }
        poke((volatile u16 *)&vl, lw, 2);
        poke((volatile u16 *)&vu, lw, 2);
        poke((volatile u16 *)&vi, lw, 1);
        poke((volatile u16 *)&vw, lw, 1);
        vr = 1.5;
        vr = (double)vl;
        chk("long", sw);
        vr = 1.5;
        vr = (double)vu;
        chk("ulong", uw);
        vr = 1.5;
        vr = (double)vi;
        chk("int", iw);
        vr = 1.5;
        vr = (double)vw;
        chk("uint", ww);
    }
    return done_report();
}
