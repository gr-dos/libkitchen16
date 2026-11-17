/* The data files of tests/conv (see tests/conv/README.txt): reading them
 * and reporting what differs, without any floating-point operation. */

#ifndef CONV_DATA_H
#define CONV_DATA_H

/* The bits of a double, as the words a little-endian 8086 stores. */
union dbl {
    double d;
    unsigned short w[4];
};

struct dline {
    unsigned short w[4];  /* expected bits (w[3] has the sign and exponent) */
    int nan;              /* 0, or 1 (positive NaN) or -1 (negative NaN) expected */
    unsigned off;         /* expected endptr offset */
    int erange;           /* ERANGE expected */
    char *str;            /* the input, unescaped, NUL-terminated (static buffer) */
    unsigned lineno;
};

/* Open the data file NAME (a DOS name). Returns 0, or 1 after reporting. */
int data_open(const char *name);

/* The next line: 1, or 0 at the end of the file. A line that cannot be parsed
 * is reported as a failure and skipped. */
int data_next(struct dline *l);

/* Does W hold the result L expects (bit for bit; for a NaN, a NaN of that sign)? */
int data_same(const struct dline *l, const unsigned short *w);

/* Report a failed line: W is what was got, OFF the endptr offset, ERR errno. */
void data_fail(const struct dline *l, const unsigned short *w, unsigned off, int err);

/* Report some other failure of the current line, with a message. */
void data_fail_msg(const struct dline *l, const char *msg);

/* Print "ok" if nothing failed; returns the exit status. */
int data_done(void);

#endif
