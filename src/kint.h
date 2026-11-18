/* kint.h - the integral part of a double, on the bits (floor, ceil and
 * modf). */

#include "k.h"

/* x has 0 <= e < 52, e its unbiased exponent: clears the 52 - e fraction
 * bits of u and returns them in frac (4 words) ; returns 1 if any was set */
static int ktrunc(kdbl *u, int e, word *frac)
{
    int fb = 52 - e, i, nb, any = 0;
    word mask;

    for (i = 0; i < 4; i++) {
        nb = fb - 16 * i;
        frac[i] = 0;
        if (nb <= 0)
            continue;
        mask = nb >= 16 ? 0xFFFF : (word)((1u << nb) - 1);
        frac[i] = u->w[i] & mask;
        if (frac[i])
            any = 1;
        u->w[i] &= (word)~mask;
    }
    return any;
}

/* add one unit at bit fb (= 52 - e) of u: the next integer in magnitude */
#ifdef KINT_BUMP
static void kbump(kdbl *u, int e)
{
    int fb = 52 - e, i = fb >> 4;
    unsigned long c = 1ul << (fb & 15);

    for (; i < 4 && c; i++) {
        c += u->w[i];
        u->w[i] = (word)c;
        c >>= 16;
    }
}
#endif
