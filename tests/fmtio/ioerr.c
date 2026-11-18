/* Errors and limits of the stream functions (docs/spec/fmtio.md, "Behaviour"
 * and "Tests"): a null stream, a stream closed before the call, a stream
 * open for reading, a write stream whose handle is read-only or closed (a
 * full disk is not available under DOSBox-X), and the 32767-character
 * limit, where all the characters are written and the result is -1 with
 * ERANGE. After a failed write the functions return -1 with errno as
 * libc16's fwrite left it (compared here with a direct fwrite to the same
 * stream). */

#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <kitchen16.h>
#include "check.h"

#define F1 "IOERR1.TMP"
#define F2 "IOERR2.TMP"

static int wrap_vfprintf(FILE *fp, const char *f, ...)
{
    va_list ap;

    va_start(ap, f);
    return vfprintf(fp, f, ap);
}

static long length_of(const char *name)
{
    int fd = open(name, O_RDONLY | O_BINARY);
    long n;

    if (fd < 0)
        return -1;
    n = filelength(fd);
    close(fd);
    return n;
}

/* every write to fp fails: fprintf and vfprintf return -1 with the errno
 * a direct fwrite leaves, and write nothing */
static void failing(FILE *fp)
{
    int e, r;

    errno = 0;
    CHECK(fwrite("x", 1, 1, fp) == 0);
    e = errno;
    errno = 0;
    r = fprintf(fp, "abc%d", 12);
    CHECK(r < 0);
    CHECK(errno == e);
    errno = 0;
    r = wrap_vfprintf(fp, "%s|%5d", "abc", 12);
    CHECK(r < 0);
    CHECK(errno == e);
    errno = 0;
    r = fprintf(fp, "%.1000d", 7);    /* long enough for several blocks */
    CHECK(r < 0);
    CHECK(errno == e);
}

int main(void)
{
    FILE *fp;
    int r, rd, saved;

    /* a null stream is an error, not a trap */
    CHECK(fprintf((FILE *)0, "abc%d", 1) < 0);
    CHECK(wrap_vfprintf((FILE *)0, "abc%d", 1) < 0);

    /* a stream closed before the call */
    fp = fopen(F1, "wb");
    CHECK(fp != 0);
    CHECK(fprintf(fp, "ab") == 2);
    CHECK(fclose(fp) == 0);
    CHECK(fprintf(fp, "should not be written %d", 1) < 0);
    CHECK(wrap_vfprintf(fp, "nor this %s", "one") < 0);
    CHECK(length_of(F1) == 2L);

    /* a stream open for reading */
    fp = fopen(F1, "rb");
    CHECK(fp != 0);
    CHECK(fprintf(fp, "nothing %d", 1) < 0);
    CHECK(wrap_vfprintf(fp, "nothing %d", 1) < 0);
    fclose(fp);
    CHECK(length_of(F1) == 2L);

    /* a write stream whose handle is read-only (opened with open) */
    fp = fopen(F2, "wb");
    CHECK(fp != 0);
    saved = fp->_fd;
    rd = open(F1, O_RDONLY | O_BINARY);
    CHECK(rd >= 0);
    fp->_fd = rd;
    failing(fp);
    CHECK(ferror(fp) != 0);
    fp->_fd = saved;
    close(rd);
    clearerr(fp);
    CHECK(fclose(fp) == 0);
    CHECK(length_of(F1) == 2L && length_of(F2) == 0L);

    /* a write stream whose handle is closed */
    fp = fopen(F2, "wb");
    CHECK(fp != 0);
    CHECK(close(fp->_fd) == 0);
    failing(fp);
    CHECK(ferror(fp) != 0);
    fclose(fp);    /* its handle is already closed */

    /* the limit: 32767 characters are fine, more are written and fail with ERANGE */
    fp = fopen(F2, "wb");
    CHECK(fp != 0);
    errno = 0;
    r = fprintf(fp, "%.32766d%c", 1, 'x');
    CHECK(r == 32767);
    r = wrap_vfprintf(fp, "%.32767d", 1);
    CHECK(r == 32767);
    errno = 0;
    r = fprintf(fp, "%.32767dx", 1);
    CHECK(r == -1 && errno == ERANGE);
    errno = 0;
    r = fprintf(fp, "%.30000d%.30000d", 1, 2);
    CHECK(r == -1 && errno == ERANGE);
    errno = 0;
    r = wrap_vfprintf(fp, "%.20000d%.20000d", 1, 2);
    CHECK(r == -1 && errno == ERANGE);
    CHECK(fprintf(fp, "ok%d", 5) == 3);
    CHECK(fclose(fp) == 0);
    CHECK(length_of(F2) == 32767L + 32767L + 32768L + 60000L + 40000L + 3L);

    remove(F1);
    remove(F2);
    return done();
}
