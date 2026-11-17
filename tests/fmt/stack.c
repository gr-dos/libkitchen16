/* stack FILE: the stack bound (docs/spec/fmt.md, "Memory and stack"). Run
 * with `option stack=2048` (rules.mk). Each record of FILE (the cases of
 * stack.dat: %.1000f of the smallest subnormal and the like) is converted
 * as fmt does (the output is checked), and then once more with the stack
 * below the caller filled with a pattern, to find how deep vsnprintf went:
 * at most 900 bytes above its caller's frame (the call's own arguments and
 * return address allowed for), and the 2,048-byte stack is not used up. */

#define DRIVER_MEMCHECK
#include "lib/driver.h"

extern char __cdecl __libc16_stack_top[];

static unsigned get_sp(void);
#pragma aux get_sp = "mov ax, sp" value [ax];

#define STACKSIZE 2048u
#define LIMIT     920u    /* 900, and the arguments and return address of the call */

static unsigned deepest;

/* Calls vsnprintf on the record with the stack below this frame filled
 * with a pattern; returns the number of bytes of stack used below the
 * stack pointer at the call. */
static unsigned measure(struct rec *r, unsigned room)
{
    unsigned sp = get_sp();
    unsigned top = (unsigned)__libc16_stack_top;
    unsigned char *base = (unsigned char *)__libc16_stack_top;
    unsigned char *lo = base - STACKSIZE;
    unsigned char *hi = base - (top - sp) - 64;
    unsigned char *p;
    unsigned used, i;
    int ret;

    for (p = lo; p < hi; p++)
        *p = PATTERN;
    ret = call_vsnprintf((char *)area + GUARD, room, r->fmt, r->w);
    for (p = lo; p < hi && *p == PATTERN; p++)
        ;
    used = p == hi ? 64 : sp - (top - (unsigned)(base - p));
    for (i = 0; i < 16; i++)
        if (lo[i] != PATTERN)
            used = 0xFFFF;    /* the bottom of the stack was reached */
    if (ret != r->ret) {
        fail_head("stack");
        t_puts("return value\r\n");
    }
    return used;
}

int main(int argc, char **argv)
{
    FILE *fp;
    struct rec r;
    int rc;
    unsigned count = 0, used, room;

    CHECK(argc >= 1);
    if (argc < 2) {
        t_puts("usage: stack FILE\r\n");
        return 2;
    }
    fp = fopen(argv[1], "rb");
    if (fp == 0) {
        t_puts("cannot open the data file\r\n");
        return 2;
    }
    while ((rc = next_rec(fp, &r)) > 0) {
        run_rec(&r, 1);
        room = r.wantlen + 1 + 16;
        used = measure(&r, room);
        if (used > deepest)
            deepest = used;
        if (used > LIMIT) {
            fail_head("stack");
            t_puts("used ");
            t_putu(used);
            t_puts(" bytes, more than ");
            t_putu(LIMIT);
            t_puts("\r\n");
        }
        count++;
    }
    fclose(fp);
    if (rc < 0 || count == 0) {
        fail_head("data");
        t_puts("bad or empty file\r\n");
    }
    t_puts("deepest ");
    t_putu(deepest);
    t_puts(" of ");
    t_putu(LIMIT);
    t_puts(" bytes\r\n");
    return done();
}
