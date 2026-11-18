/* Register conventions of the helpers (docs/spec/fpc.md, "Registers",
 * "Memory models"), for every helper: with markers in SI, DI, BP and ES, DS
 * set to another segment, the 8-byte operand at SS:SI (the F forms) or at
 * ES:SI (the E forms, compact and large models only), the other of the two
 * holding a decoy, and the direction flag set or clear.
 *
 * lib/regscall.asm makes each call, near in the small and compact models
 * and far in the large model, and reports every register afterwards. Checked:
 *   - SI DI BP DS ES SS SP are as before, the direction flag as before;
 *   - the operand slots are unchanged;
 *   - the stack below the helper's 64 bytes is untouched (the 192 bytes below
 *     SP hold a pattern, with interrupts off);
 *   - the results are the ones the compiler's own call sites give for the
 *     same operands (so this program does not need a corpus);
 *   - __FDC and __EDC leave AX BX CX DX unchanged, and set the flags as for
 *     cmp of 0 with 1, 0 with 0 and 1 with 0, an unordered pair like >.
 *
 * Only AX BX CX DX (and the flags) may change otherwise, so those are not
 * checked except for the results. */

#include "lib/report.h"

struct blk {
    unsigned op;
    unsigned in[4];             /* AX BX CX DX */
    unsigned pad1[3];
    unsigned s0[4];             /* at +16: SS:SI */
    unsigned pad2[4];
    unsigned s1[4];             /* at +32: ES:SI */
    unsigned out[12];           /* AX BX CX DX SI DI BP DS ES SS SP flags */
    unsigned guard;
    unsigned status;
    unsigned mdi, mbp, mds, mdf;
    unsigned esi, ees, ess, esp;
};

void __cdecl fpc_call(struct blk *p);

enum { FDA, FDS, FDM, FDD, FDC, FDN, FDI4, FDU4, I4FD, U4FD, FDFS, FSFD, EDA, EDS, EDM, EDD, EDC };

static struct blk b;

/* doubles, most significant word first */
static const unsigned dv[16][4] = {
    { 0x3FF0, 0x0000, 0x0000, 0x0000 },     /* 1 */
    { 0xC004, 0x0000, 0x0000, 0x0000 },     /* -2.5 */
    { 0x3FB9, 0x9999, 0x9999, 0x999A },     /* 0.1 */
    { 0x0000, 0x0000, 0x0000, 0x0000 },     /* +0 */
    { 0x8000, 0x0000, 0x0000, 0x0000 },     /* -0 */
    { 0x7FF0, 0x0000, 0x0000, 0x0000 },     /* +inf */
    { 0x7FF8, 0x0000, 0x0000, 0x0000 },     /* NaN */
    { 0x7FEF, 0xFFFF, 0xFFFF, 0xFFFF },     /* max */
    { 0x0000, 0x0000, 0x0000, 0x0001 },     /* min subnormal */
    { 0x0010, 0x0000, 0x0000, 0x0000 },     /* min normal */
    { 0x7E37, 0xE43C, 0x8800, 0x759C },     /* 1e300 */
    { 0x01A5, 0x6E1F, 0xC2F8, 0xF359 },     /* 1e-300 */
    { 0x41EF, 0xFFFF, 0xFFFF, 0x8000 },     /* 4294967295.5 */
    { 0xC1E0, 0x0000, 0x0000, 0x0000 },     /* -2147483648 */
    { 0x3FF0, 0x0000, 0x0000, 0x0001 },     /* 1 + 2^-52 */
    { 0x47EF, 0xFFFF, 0xE000, 0x0000 }      /* FLT_MAX */
};

/* 32-bit integers and floats, most significant word first */
static const unsigned lv[16][2] = {
    { 0x0000, 0x0000 }, { 0x0000, 0x0001 }, { 0xFFFF, 0xFFFF }, { 0x7FFF, 0xFFFF },
    { 0x8000, 0x0000 }, { 0x1234, 0x5678 }, { 0x0000, 0x8000 }, { 0x0000, 0xFFFF },
    { 0x0001, 0x0000 }, { 0xFFFF, 0x8000 }, { 0x3F80, 0x0000 }, { 0xBF80, 0x0000 },
    { 0x7F80, 0x0000 }, { 0x7FC0, 0x0000 }, { 0x0000, 0x0001 }, { 0x7F7F, 0xFFFF }
};

static volatile double ra, rb, rr;
static volatile long rl;
static volatile unsigned long ru;
static volatile float rf;

/* the correct results, from the compiler's call sites */
static unsigned want[4];
static int want_n;      /* words of the result in want: 4, 2, or 0 for flags */
static int want_rel;    /* for the comparisons: '<', '=' or '>' */

static void put_name(int op)
{
    static const char *const names[] = {
        "FDA", "FDS", "FDM", "FDD", "FDC", "FDN", "FDI4", "FDU4", "I4FD", "U4FD", "FDFS", "FSFD",
        "EDA", "EDS", "EDM", "EDD", "EDC"
    };
    t_puts(names[op]);
}

static void report(int op, const char *what, unsigned i, unsigned j)
{
    if (begin_fail("regs")) {
        put_name(op);
        t_puts(" ");
        t_puts(what);
        t_puts(" case ");
        t_putu(i);
        t_puts(",");
        t_putu(j);
        t_puts(" df=");
        t_putu(b.mdf);
        t_puts("\r\n");
    }
}

static unsigned save0[4], save1[4];

/* make the call for op with the operands in the slots already set, and check
 * the registers */
static void run(int op, unsigned i, unsigned j, unsigned k)
{
    static const unsigned mdi[2] = { 0xD1D1, 0x5A5A };
    static const unsigned mbp[2] = { 0xB9B9, 0xA5A5 };
    static const unsigned mds[2] = { 0x1357, 0x2468 };
    unsigned f, n;

    b.op = op;
    b.mdi = mdi[k & 1];
    b.mbp = mbp[k & 1];
    b.mds = mds[(k >> 1) & 1];
    b.mdf = (k >> 2) & 1;
    for (n = 0; n < 4; n++) {
        save0[n] = b.s0[n];
        save1[n] = b.s1[n];
    }
    nchecks++;
    fpc_call(&b);
    if (b.status != 0) {
        report(op, "SS is not DS", i, j);
        return;
    }
    if (b.out[4] != b.esi) report(op, "SI changed", i, j);
    if (b.out[5] != b.mdi) report(op, "DI changed", i, j);
    if (b.out[6] != b.mbp) report(op, "BP changed", i, j);
    if (b.out[7] != b.mds) report(op, "DS changed", i, j);
    if (b.out[8] != b.ees) report(op, "ES changed", i, j);
    if (b.out[9] != b.ess) report(op, "SS changed", i, j);
    if (b.out[10] != b.esp) report(op, "SP changed", i, j);
    f = b.out[11];
    if (((f >> 10) & 1) != b.mdf) report(op, "direction flag changed", i, j);
    if (b.guard != 1) report(op, "used more than 64 bytes of stack", i, j);
    for (n = 0; n < 4; n++)
        if (save0[n] != b.s0[n] || save1[n] != b.s1[n]) {
            report(op, "wrote its operand", i, j);
            break;
        }
    if (op == FDC || op == EDC) {
        int zf = (f >> 6) & 1, cf = f & 1, sf = (f >> 7) & 1, of = (f >> 11) & 1;
        int rel = zf ? '=' : cf ? '<' : '>';

        for (n = 0; n < 4; n++)
            if (b.out[n] != b.in[n])
                report(op, "AX..DX changed", i, j);
        if (rel != want_rel)
            report(op, "wrong relation", i, j);
        else if (rel == '<' ? !(sf && !of) : rel == '=' ? (sf || of) : (sf != of))
            report(op, "flags are not those of cmp", i, j);
    } else if (want_n == 4) {
        if (b.out[0] != want[3] || b.out[1] != want[2] || b.out[2] != want[1] || b.out[3] != want[0])
            report(op, "wrong result", i, j);
    } else if (want_n == 2) {
        if (b.out[0] != want[0] || b.out[3] != want[1])
            report(op, "wrong result", i, j);
    }
}

/* the operand at b (words, least significant first) goes to slot 0 for an F
 * form and to slot 1 for an E form, the other slot gets its complement */
static void set_operand(const unsigned *w, int e)
{
    unsigned n;
    unsigned *t = e ? b.s1 : b.s0;
    unsigned *d = e ? b.s0 : b.s1;

    for (n = 0; n < 4; n++) {
        t[n] = w[n];
        d[n] = ~w[n];
    }
}

static void get_d(volatile double *p, unsigned *w)
{
    const volatile unsigned *q = (const volatile unsigned *)p;
    unsigned n;

    for (n = 0; n < 4; n++)
        w[n] = q[n];
}

static void put_d(volatile double *p, const unsigned *w)
{
    volatile unsigned *q = (volatile unsigned *)p;
    unsigned n;

    for (n = 0; n < 4; n++)
        q[n] = w[n];
}

static void load_d(unsigned i, unsigned *w)     /* dv[i] least significant first */
{
    unsigned n;

    for (n = 0; n < 4; n++)
        w[n] = dv[i][3 - n];
}

static void binary(int op, int e, unsigned i, unsigned j, unsigned k)
{
    unsigned aw[4], bw[4];
    int base = op - (e ? EDA - FDA : 0);

    load_d(i, aw);
    load_d(j, bw);
    put_d(&ra, aw);
    put_d(&rb, bw);
    want_n = 4;
    if (base == FDC) {
        want_n = 0;
        want_rel = ra == rb ? '=' : ra < rb ? '<' : '>';
    } else {
        rr = 0.0;
        if (base == FDA) rr = ra + rb;
        else if (base == FDS) rr = ra - rb;
        else if (base == FDM) rr = ra * rb;
        else rr = ra / rb;
        get_d(&rr, want);
    }
    set_operand(bw, e);
    b.in[0] = dv[i][0];
    b.in[1] = dv[i][1];
    b.in[2] = dv[i][2];
    b.in[3] = dv[i][3];
    run(op, i, j, k);
}

static void unary(int op, unsigned i, unsigned k)
{
    unsigned aw[4];

    load_d(i, aw);
    put_d(&ra, aw);
    b.in[0] = dv[i][0];
    b.in[1] = dv[i][1];
    b.in[2] = dv[i][2];
    b.in[3] = dv[i][3];
    set_operand(aw, 0);                  /* nothing reads the slots: decoys */
    want_n = 4;
    if (op == FDN) {
        rr = 0.0;
        rr = -ra;
        get_d(&rr, want);
    } else if (op == FDI4) {
        rl = 0;
        rl = (long)ra;
        want_n = 2;
        want[0] = ((volatile unsigned *)&rl)[0];
        want[1] = ((volatile unsigned *)&rl)[1];
    } else if (op == FDU4) {
        ru = 0;
        ru = (unsigned long)ra;
        want_n = 2;
        want[0] = ((volatile unsigned *)&ru)[0];
        want[1] = ((volatile unsigned *)&ru)[1];
    } else {                             /* FDFS */
        rf = 0.0f;
        rf = (float)ra;
        want_n = 2;
        want[0] = ((volatile unsigned *)&rf)[0];
        want[1] = ((volatile unsigned *)&rf)[1];
    }
    run(op, i, 0, k);
}

/* the 32-bit operand in DX:AX */
static void from32(int op, unsigned i, unsigned k)
{
    unsigned w[4];

    ((volatile unsigned *)&rl)[0] = lv[i][1];
    ((volatile unsigned *)&rl)[1] = lv[i][0];
    ((volatile unsigned *)&ru)[0] = lv[i][1];
    ((volatile unsigned *)&ru)[1] = lv[i][0];
    ((volatile unsigned *)&rf)[0] = lv[i][1];
    ((volatile unsigned *)&rf)[1] = lv[i][0];
    rr = 0.0;
    if (op == I4FD) rr = (double)rl;
    else if (op == U4FD) rr = (double)ru;
    else rr = (double)rf;
    get_d(&rr, want);
    want_n = 4;
    b.in[0] = lv[i][1];
    b.in[3] = lv[i][0];
    b.in[1] = 0x1111;
    b.in[2] = 0x2222;
    load_d(i, w);
    set_operand(w, 0);
    run(op, i, 0, k);
}

int main(void)
{
    unsigned i, j, k;
    int op;

    for (op = FDA; op <= FDC; op++)
        for (i = 0; i < 16; i++)
            for (j = 0; j < 16; j++) {
                k = (i * 3 + j * 5 + (i >> 2)) & 7;
                binary(op, 0, i, j, k);
#if defined(__COMPACT__) || defined(__LARGE__)
                binary(op + (EDA - FDA), 1, i, j, (k + 3) & 7);
#endif
            }
    for (i = 0; i < 16; i++)
        for (k = 0; k < 8; k++) {
            unary(FDN, i, k);
            unary(FDI4, i, k);
            unary(FDU4, i, k);
            unary(FDFS, i, k);
            from32(I4FD, i, k);
            from32(U4FD, i, k);
            from32(FSFD, i, k);
        }
    return done_report();
}
