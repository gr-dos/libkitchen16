/* nofpu [fpu]: checks that no floating-point coprocessor is present, so the
 * other suites really run without one (decision 5; tests/run-dos sets
 * DOSBox-X's fpu=false). With `fpu`, checks the opposite instead, which is
 * how the probe itself is tested: run it under DOSBox-X with fpu=true.
 *
 * The probe is the usual one: FNINIT, then FNSTSW to a word holding a
 * pattern. With a coprocessor the status word after FNINIT is 0. Without
 * one, an 8086 treats both ESC instructions as no-ops (FNSTSW only reads
 * its memory operand), and the pattern is left. The two instructions are
 * written as bytes, and this program's object is the one test object
 * scripts/check-8086 does not check (Makefile, `test`). */

#include <dos.h>
#include "check.h"

static unsigned status;

/* FNINIT (DB E3); FNSTSW [BX] (DD 3F). DS is DGROUP in every model, where
 * `status` is. */
static void probe(unsigned offset);
#pragma aux probe = 0xDB 0xE3 0xDD 0x3F parm [bx] modify exact [];

int main(int argc, char **argv)
{
    int want_fpu = argc > 1 && argv[1][0] == 'f';
    unsigned *p = &status;

    status = 0x5A5Au;
    probe(FP_OFF(p));
    if (want_fpu)
        CHECK(status == 0);
    else
        CHECK(status == 0x5A5Au);
    return done();
}
