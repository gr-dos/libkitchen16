tests/fmtio - formatted output to a stream (docs/spec/fmtio.md)
================================================================

Run by tests/run-dos in every model. Files:

  cases          the programs run (NAME STATUS PROGRAM [ARGUMENTS])
  members        the members each program links (tests/check-members); the
                 libkitchen16 names are `fmt` (the engine), vfprintf, fprintf,
                 vprintf, printf, sprintf; libc16's have the c16: prefix.
                 The implementation may need these names adjusted.
  pc.expect      the expected standard output of `fmtio PC.DAT stdout`
                 (written by tests/fmt/gen, with data/pc.dat)
  data/          links to tests/fmt/data (the corpus is the same files; run-dos
                 copies what they point to) and pc.dat

Programs
--------

  fmtio FILE [stdout]
      FILE is a data file of tests/fmt (format in tests/fmt/lib/driver.h).
      Without `stdout`: every line is written to the binary file FMTIO.TMP
      with fprintf (even lines) or vfprintf (odd lines), the return value is
      checked for every line, the file is closed, read back and compared byte
      for byte with the expected outputs in order, which are the fourth field of
      the data file (the generator writes no second copy of them). With
      `stdout`: the lines go to standard output through printf and vprintf,
      each followed by `|` and a line end, and run-dos compares them with
      pc.expect.
  ioerr
      A null stream; a stream closed before the call; a stream open for
      reading; a write stream whose handle is read-only (opened with open;
      FILE's _fd is replaced); one whose handle is closed. In each: -1 (any
      negative), no output, and errno as a direct fwrite to the same stream
      leaves it. Then the limit: 32767 characters are fine, 32768 and
      `%.30000d%.30000d` are written whole and return -1 with ERANGE, and the
      file's length is checked.
  blocks
      Defines its own fwrite (counts calls, keeps what it is given, can fail
      from the nth call on, or write fewer characters than asked), which links
      before libc16's. 1,000 characters of literal text, of a %s, of padding
      (%1000d %-1000d %.1000d %01000d), of a long fraction (%.998f) and of 250
      small conversions: the content is exact, no block is over 128 characters
      and there are at most 8 calls, for fprintf, vfprintf, printf and vprintf;
      after a failed call (no characters, or fewer, written) the function stops
      (no further call) and returns a negative number.
  nostream, only_fp, only_vfp, only_pf, only_vpf
      One function each: `members` says what links: the stream functions link
      vfprintf, the engine, libc16's fwrite (and what it links) and stdout; a
      program with sprintf only links none of it.

Runtime
-------

Each fmtio case writes and reads back the lines of one data file (about 12,000
lines in all); they are cheap next to the fmt suite, except dblexact, dblcore,
dblrand and dblxe (exact conversions), and ioerr, which formats about 250,000
characters. See tests/fmt/README.txt for the speed of DOSBox-X under run-dos.
