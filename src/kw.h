/* kw.h - multi-word integers (16-bit words, least significant first), for
 * the members that work on the bits of a double. A member defines the KW_
 * macros of the functions it uses before including this, so that nothing
 * is compiled that is not referenced. */

#ifndef KW_H
#define KW_H

#ifdef KW_CLR
static void kw_clr(word *m, int n)
{
    int i;
    for (i = 0; i < n; i++)
        m[i] = 0;
}
#endif

#ifdef KW_CPY
static void kw_cpy(word *d, const word *s, int n)
{
    int i;
    for (i = 0; i < n; i++)
        d[i] = s[i];
}
#endif

#ifdef KW_NZ
static int kw_nz(const word *m, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (m[i])
            return 1;
    return 0;
}
#endif

/* the index of the highest set bit, or -1 */
#ifdef KW_TOP
static int kw_top(const word *m, int n)
{
    int i, b;
    word v;
    for (i = n - 1; i >= 0; i--)
        if (m[i]) {
            v = m[i];
            b = 15;
            while (!(v & 0x8000)) {
                v <<= 1;
                b--;
            }
            return i * 16 + b;
        }
    return -1;
}
#endif

/* shift right by k >= 0 bits; returns 1 if a set bit was shifted out */
#ifdef KW_SHR
static int kw_shr(word *m, int n, int k)
{
    int ws, bs, i, sticky = 0;
    unsigned lo, hi;

    if (k <= 0)
        return 0;
    if (k >= 16 * n) {
        for (i = 0; i < n; i++) {
            if (m[i])
                sticky = 1;
            m[i] = 0;
        }
        return sticky;
    }
    ws = k >> 4;
    bs = k & 15;
    for (i = 0; i < ws; i++)
        if (m[i])
            sticky = 1;
    if (bs && (m[ws] & ((1u << bs) - 1)))
        sticky = 1;
    for (i = 0; i < n; i++) {
        lo = (i + ws < n) ? m[i + ws] : 0;
        hi = (i + ws + 1 < n) ? m[i + ws + 1] : 0;
        m[i] = bs ? (word)((lo >> bs) | (hi << (16 - bs))) : (word)lo;
    }
    return sticky;
}
#endif

/* shift left by k >= 0 bits (bits shifted out are lost) */
#ifdef KW_SHL
static void kw_shl(word *m, int n, int k)
{
    int ws, bs, i;
    unsigned hi, lo;

    if (k <= 0)
        return;
    if (k >= 16 * n) {
        for (i = 0; i < n; i++)
            m[i] = 0;
        return;
    }
    ws = k >> 4;
    bs = k & 15;
    for (i = n - 1; i >= 0; i--) {
        hi = (i - ws >= 0) ? m[i - ws] : 0;
        lo = (i - ws - 1 >= 0) ? m[i - ws - 1] : 0;
        m[i] = bs ? (word)((hi << bs) | (lo >> (16 - bs))) : (word)hi;
    }
}
#endif

/* -1, 0, 1 */
#ifdef KW_CMP
static int kw_cmp(const word *a, const word *b, int n)
{
    int i;
    for (i = n - 1; i >= 0; i--)
        if (a[i] != b[i])
            return a[i] < b[i] ? -1 : 1;
    return 0;
}
#endif

#ifdef KW_ADD
static void kw_add(word *a, const word *b, int n)
{
    unsigned long c = 0;
    int i;
    for (i = 0; i < n; i++) {
        c += (unsigned long)a[i] + b[i];
        a[i] = (word)c;
        c >>= 16;
    }
}
#endif

/* a - b, for a >= b */
#ifdef KW_SUB
static void kw_sub(word *a, const word *b, int n)
{
    int i, borrow = 0, bo;
    unsigned x, y;
    for (i = 0; i < n; i++) {
        x = a[i];
        y = b[i];
        bo = (x < y) || (x == y && borrow);
        a[i] = (word)(x - y - borrow);
        borrow = bo;
    }
}
#endif

/* add 1 */
#ifdef KW_INC
static void kw_inc(word *a, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (++a[i])
            break;
}
#endif

/* the 16 x 16 -> 32 bit product, without a library call */
#ifdef KW_MUL
#ifdef K_HOST
static unsigned long mul16(unsigned a, unsigned b)
{
    return (unsigned long)a * b;
}
#else
unsigned long mul16(unsigned a, unsigned b);
#pragma aux mul16 = "mul dx" parm [ax] [dx] value [dx ax];
#endif
#endif

#endif
