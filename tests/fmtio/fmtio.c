/* fmtio FILE [stdout]: the corpus through the stream functions
 * (docs/spec/fmtio.md, "Tests").
 *
 * Without `stdout`, each P record of FILE (a data file of tests/fmt, see
 * ../fmt/lib/driver.h) is written to the file FMTIO.TMP, by fprintf for the
 * even lines and vfprintf for the odd ones, and the return value is
 * checked; then the file is closed, read back, and compared byte for byte
 * with the expected output of every record in turn (the stream is binary,
 * so that characters such as \n and \r are compared as they are).
 *
 * With `stdout`, each record goes to standard output by printf (even lines)
 * or vprintf (odd), followed by `|` and a line end, for the .expect file
 * next to the cases (tests/fmtio/pc.expect, written by tests/fmt/gen). */

#define DRIVER_STREAMS
#include "../fmt/lib/driver.h"

#define TMP "FMTIO.TMP"

static unsigned char back[1600];

static void report(const char *what, struct rec *r, int ret)
{
    if (shown < MAXSHOW) {
        fail_head(what);
        t_puts("fmt ");
        put_esc(r->fmt, (unsigned)strlen(r->fmt));
        t_puts(" ret ");
        put_int(ret);
        t_puts(" want ");
        put_int(r->ret);
        t_puts("\r\n");
    } else {
        failures++;
    }
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    struct rec r;
    int rc, ret, to_stdout;
    unsigned count = 0;

    CHECK(argc >= 1);
    if (argc < 2) {
        t_puts("usage: fmtio FILE [stdout]\r\n");
        return 2;
    }
    to_stdout = argc > 2 && argv[2][0] == 's';
    in = fopen(argv[1], "rb");
    if (in == 0) {
        t_puts("cannot open the data file\r\n");
        return 2;
    }

    if (to_stdout) {
        while ((rc = next_rec(in, &r)) > 0) {
            if (r.mode != 'P')
                continue;
            ret = (lineno & 1) ? call_vprintf(r.fmt, r.w) : call_printf(r.fmt, r.w, r.nw);
            t_puts("|\r\n");
            if (ret != r.ret)
                report("printf", &r, ret);
            count++;
        }
        fclose(in);
        if (rc < 0 || count == 0) {
            fail_head("data");
            t_puts("bad or empty file\r\n");
        }
        return done();
    }

    /* write */
    out = fopen(TMP, "wb");
    if (out == 0) {
        t_puts("cannot create the output file\r\n");
        return 2;
    }
    while ((rc = next_rec(in, &r)) > 0) {
        if (r.mode != 'P')
            continue;
        ret = (lineno & 1) ? call_vfprintf(out, r.fmt, r.w) : call_fprintf(out, r.fmt, r.w, r.nw);
        if (ret != r.ret || ferror(out))
            report((lineno & 1) ? "vfprintf" : "fprintf", &r, ret);
        count++;
    }
    if (rc < 0 || count == 0) {
        fail_head("data");
        t_puts("bad or empty file\r\n");
    }
    CHECK(fclose(out) == 0);
    fclose(in);

    /* read back and compare */
    in = fopen(argv[1], "rb");
    out = fopen(TMP, "rb");
    if (in == 0 || out == 0) {
        t_puts("cannot reopen the files\r\n");
        return 2;
    }
    lineno = 0;
    while ((rc = next_rec(in, &r)) > 0) {
        unsigned got;

        if (r.mode != 'P')
            continue;
        got = r.wantlen ? (unsigned)fread(back, 1, r.wantlen, out) : 0;
        if (got != r.wantlen || memcmp(back, r.want, r.wantlen) != 0) {
            fail_head("file");
            t_puts("output differs, fmt ");
            put_esc(r.fmt, (unsigned)strlen(r.fmt));
            t_puts(" want ");
            put_esc(r.want, r.wantlen);
            t_puts(" got ");
            put_esc((const char *)back, got);
            t_puts("\r\n");
            break;
        }
    }
    CHECK(fread(back, 1, 1, out) == 0);    /* nothing after the last record */
    fclose(in);
    fclose(out);
    remove(TMP);
    if (failures) {
        t_putu(failures);
        t_puts(" failures in ");
        t_putu(count);
        t_puts(" records\r\n");
    }
    return done();
}
