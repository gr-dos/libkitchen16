/* The six C comparisons of doubles (docs/spec/fpc.md, "Comparison"), against
 * CMP.DAT: each row is "A B C" with C the relation of a to b, < = or >, an
 * unordered pair counting as >, so that < <= == != > >= give the results
 * the flags of __FDC / __EDC make the compiler's jumps produce.
 *
 * Every comparison is made in four ways, so that every call-site shape is
 * used: both operands in global objects (g), the right operand in a local
 * (l), both through pointers (p: far in the compact and large models, so the
 * E forms) and the left operand in the object with the right through a
 * pointer (m). The failing comparison is named in the tag, for example
 * "p<=". */

#include "lib/rows.h"

static volatile double va, vb;
static volatile double *volatile pa = &va;
static volatile double *volatile pb = &vb;

static u16 aw[4], bw[4];
static char rel;

static const char names[6][3] = { "<", "<=", "==", "!=", ">", ">=" };

/* the expected results of < <= == != > >= for each relation */
static void expect(int *e)
{
    e[0] = rel == '<';
    e[1] = rel != '>';
    e[2] = rel == '=';
    e[3] = rel != '=';
    e[4] = rel == '>';
    e[5] = rel != '<';
}

static void chk(char form, const int *r, const int *e)
{
    int i;

    for (i = 0; i < 6; i++) {
        nchecks++;
        if ((r[i] != 0) != e[i]) {
            u16 g[1];
            char tag[5];

            tag[0] = form;
            tag[1] = names[i][0];
            tag[2] = names[i][1];
            tag[3] = '\0';
            g[0] = r[i] != 0;
            fail_row(tag, g, 1);
        }
    }
}

int main(void)
{
    int an, bn;

    if (!open_data("CMP.DAT"))
        return 1;
    while (next_row()) {
        const char *p = get_field(row, aw, 4, &an);
        int e[6], r[6];

        if (p)
            p = get_field(p, bw, 4, &bn);
        if (p)
            p = get_char(p, &rel);
        if (p == NULL || an || bn || (rel != '<' && rel != '=' && rel != '>')) {
            bad_row();
            continue;
        }
        expect(e);
        poke((volatile u16 *)&va, aw, 4);
        poke((volatile u16 *)&vb, bw, 4);

        r[0] = va < vb; r[1] = va <= vb; r[2] = va == vb; r[3] = va != vb; r[4] = va > vb; r[5] = va >= vb;
        chk('g', r, e);
        {
            volatile double lb = vb;

            r[0] = va < lb; r[1] = va <= lb; r[2] = va == lb; r[3] = va != lb; r[4] = va > lb; r[5] = va >= lb;
            chk('l', r, e);
        }
        r[0] = *pa < *pb; r[1] = *pa <= *pb; r[2] = *pa == *pb; r[3] = *pa != *pb; r[4] = *pa > *pb; r[5] = *pa >= *pb;
        chk('p', r, e);
        r[0] = va < *pb; r[1] = va <= *pb; r[2] = va == *pb; r[3] = va != *pb; r[4] = va > *pb; r[5] = va >= *pb;
        chk('m', r, e);
    }
    return done_report();
}
