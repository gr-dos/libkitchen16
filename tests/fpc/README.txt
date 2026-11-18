tests/fpc: tests of the floating-point helpers (docs/spec/fpc.md)

Run by `make test` (tests/run-dos with the cases file) in each model. The
programs are compiled with -fpc, so the compiler's own call sites exercise
the helpers: __FDA __FDS __FDM __FDD __FDC __FDN __FDI4 __FDU4 __I4FD
__U4FD __FDFS __FSFD, and, in the compact and large models, __EDA __EDS
__EDM __EDD __EDC (a right operand reached through a far pointer).

Files

  cases            one case per program run (name, status 0, program, arguments)
  NAME.expect      the output of each case: the single line "ok". A program
                   prints only failing rows (at most 12, "not ok TAG ROW: the
                   data row got BITS") and then the number of failures, or
                   "ok" if there were none.
  arith.c          add|sub|mul|div|neg: ADD.DAT ... NEG.DAT, four call-site
                   shapes per row (globals, local right operand, both through
                   pointers, left global and right through a pointer)
  compare.c        CMP.DAT: all six C comparisons, the same four shapes
  toint.c          TOINT.DAT: (long) (unsigned long) (int) (unsigned), out of
                   range included
  fromint.c        FROMINT.DAT: (double) of long, unsigned long, int, unsigned
  tofloat.c        d2f|f2d: D2F.DAT (float)a, F2D.DAT (double)f
  regs.c           register and stack conventions of every helper (see below)
  purity.c         adds two doubles and links nothing from libc16 but cstart
  lib/report.h     failure counting and printing (testio only)
  lib/rows.h       data-file reading and hex parsing, with no floating point
  lib/regscall.asm calls one helper with chosen registers (test code)
  rules.mk         links regscall.obj into regs only
  gen              the corpus generator
  data/*.DAT       its output, committed

The corpus and how to regenerate it

  perl tests/fpc/gen            rewrite tests/fpc/data
  perl tests/fpc/gen --check    verify the files that are there, write nothing

Each data row is hexadecimal bit patterns, big-endian: 16 digits for a
double, 8 for a float or a 32-bit integer; a result "N" means any NaN (a
NaN's sign and payload are not significant, docs/spec/fpc.md). Formats are in
the header of gen. About 14,500 rows in all (ADD and SUB about 3,000 each,
MUL and DIV about 2,500, CMP 2,200, D2F 1,400, TOINT 700, FROMINT 500, F2D
500, NEG 400, about 900 KB).

The expected results come from the host's IEEE 754 arithmetic (Perl doubles,
pack and unpack), with the zero, infinity and NaN rules of the spec written
out in `special`, and with one correction for Perl's pack 'f', which turns
magnitudes between FLT_MAX and 2^128 - 2^103 into infinities. Every row is
then re-verified by a second method (exact Math::BigInt arithmetic and one
hand-written round-to-nearest-even step with gradual underflow and overflow),
and gen refuses to write anything if the two differ; `--check` repeats the
second method on the files. The corpus was also compared once with Python's
float arithmetic and struct (add, sub, mul, div, cmp, d2f). The random part
uses srand with a fixed seed (Perl's own generator, the same everywhere), so
the output is deterministic.

The corpus has: zeros of both signs, ones, powers of two, the largest and
smallest normal and subnormal numbers and their neighbours, infinities, NaNs
(quiet, signalling, negative, with payload); every pair of those; each
special with the next value above and below it (cancellation, ulp-level
cases); ties for every operation (half an ulp and one and a half ulps for
addition and subtraction; 54-bit odd products, products at the top of the
range and exact half-way results in the subnormal range for multiplication;
exact half-way subnormal quotients for division; half-way values for every
float, including FLT_MAX, FLT_MIN and the float subnormals, for double to
float); overflow and underflow boundaries; the out-of-range table of
__FDI4/__FDU4 exactly (2^31, 2^32, -2^31 - 1 and the values next to them,
NaN, both infinities); and several hundred pseudo-random operands of mixed
kinds (any bit pattern, nearby exponents, sparse mantissas, subnormals,
extreme exponents).

regs.c

lib/regscall.asm sets DS to another segment, SI to the operand slot at SS:SI,
ES so that ES:SI is a different place (a decoy for the F forms, the operand
for the E forms), markers in DI and BP, the direction flag set or clear, and
a pattern in the 192 bytes below SP with interrupts off; it calls the helper
(near in the small and compact models, far in the large model) and records
every register. regs.c checks that SI DI BP DS ES SS SP and the direction flag
are unchanged, that neither operand slot was written, that the helper used
no more than about 70 bytes of stack, that results equal those of the
compiler's own call sites, and for __FDC and __EDC that AX BX CX DX are
unchanged and that ZF CF SF OF are those of cmp 0,1 / 0,0 / 1,0 (a NaN like
1,0). The harness itself was tried against stub helpers that only return
(no failure from the registers), and against stubs that change SI, set DF
or use too much stack (each reported).

What the implementer must do

  * Create tests/fpc/members. purity.c is the one program checked for what it
    links: it adds two doubles, so its members are the library members of
    __FDA and what they share, and no libc16 member except cstart:

        purity: <the libkitchen16 members of __FDA> c16:cstart

    (docs/spec/fpc.md, "Must": the members of fpc link nothing from libc16.)
    No members file is written here, because the member names are the
    implementation's choice; once it exists, tests/check-members also checks
    purity. The other programs link libc16's stdio and startup, so they are
    not listed.
  * The small model has no E forms; regscall.asm and the programs use them
    only when __COMPACT__ or __LARGE__ is defined (the symbols that
    tests/abi/fpc.c shows).
  * regs.EXE needs all the helpers (all 12, and 5 more in compact and
    large).
