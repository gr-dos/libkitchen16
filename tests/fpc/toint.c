/* double to integer: (long), (unsigned long), (int) and (unsigned) casts
 * (docs/spec/fpc.md, "Conversions to integers"), against TOINT.DAT: each row
 * is "A I U" with I and U the 32-bit results of __FDI4 and __FDU4, out of
 * range included. (int)a and (unsigned)a are the low words of I and U. Each
 * cast is also made from a local and through a pointer. */

#include "lib/rows.h"

static volatile double va;
static volatile double *volatile pa = &va;

static volatile long rl;
static volatile unsigned long ru;
static volatile int ri;
static volatile unsigned rui;

static u16 aw[4], iw[2], uw[2];

static void chk(const char *tag, const volatile u16 *r, const u16 *want, int n)
{
    u16 got[2];

    grab(got, r, n);
    if (!same_bits(got, want, n, 0))
        fail_row(tag, got, n);
}

int main(void)
{
    int an, n1, n2;

    if (!open_data("TOINT.DAT"))
        return 1;
    while (next_row()) {
        const char *p = get_field(row, aw, 4, &an);

        if (p)
            p = get_field(p, iw, 2, &n1);
        if (p)
            p = get_field(p, uw, 2, &n2);
        if (p == NULL || an || n1 || n2) {
            bad_row();
            continue;
        }
        poke((volatile u16 *)&va, aw, 4);
        rl = 0; ru = 0; ri = 0; rui = 0;
        rl = (long)va;
        chk("long", (volatile u16 *)&rl, iw, 2);
        ru = (unsigned long)va;
        chk("ulong", (volatile u16 *)&ru, uw, 2);
        ri = (int)va;
        chk("int", (volatile u16 *)&ri, iw, 1);
        rui = (unsigned)va;
        chk("uint", (volatile u16 *)&rui, uw, 1);
        {
            volatile double la = va;

            rl = 0; ru = 0; ri = 0; rui = 0;
            rl = (long)la;
            chk("long.l", (volatile u16 *)&rl, iw, 2);
            ru = (unsigned long)la;
            chk("ulong.l", (volatile u16 *)&ru, uw, 2);
        }
        rl = 0; ru = 0;
        rl = (long)*pa;
        chk("long.p", (volatile u16 *)&rl, iw, 2);
        ru = (unsigned long)*pa;
        chk("ulong.p", (volatile u16 *)&ru, uw, 2);
    }
    return done_report();
}
