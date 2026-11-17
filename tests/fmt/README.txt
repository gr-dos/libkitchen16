tests/fmt - formatted output to memory (docs/spec/fmt.md)
==========================================================

Run by tests/run-dos in every model (make MODEL=s|c|l test, or just make test).
Files:

  cases          the programs run, one per line: NAME STATUS PROGRAM [ARGUMENTS]
  members        the libkitchen16 and libc16 members each program must link
                 (tests/check-members); `fmt` is the engine, then snprintf
                 vsnprintf sprintf vsprintf; libc16's have the c16: prefix.
                 The implementation may need these names adjusted.
  rules.mk       stack.EXE is linked with `option stack=2048`
  gen            Perl generator of data/*.dat (and tests/fmtio's pc.dat and
                 pc.expect). Committed output; regenerate with
                   perl tests/fmt/gen            (about 15 s)
                 and check the model against the host with
                   perl tests/fmt/gen --verify --out /tmp/x --fmtio /tmp/y
  data/          the corpora, copied by run-dos to the DOS directory
  lib/driver.h   the reader and the call helpers shared with tests/fmtio

Programs
--------

  fmt FILE [all]      runs a data file (below): for each line, one of sprintf,
                      vsprintf, snprintf, vsnprintf in turn (all four with
                      `all`; the N lines run the two n functions) with a target
                      buffer between guard bytes of a known pattern, and checks
                      the return value, the characters, the terminating null
                      and that no other byte changed. Prints the first failing
                      lines (line number, function, format, results, expected)
                      and a count, or `ok`.
  fastpath FILE|- [self]
                      the integral-value fast path: FILE (data/fast*.dat) is the
                      f, g, G and e conversions of integral doubles below 2^53
                      and a few above, at 17 precisions, all 32 flag
                      combinations and widths; `self` makes doubles from the
                      bits of integers (no floating-point operation) and checks
                      %.0f, %f, %.3f, %#.0f, %+f, widths, %g and %G against the
                      integer's own digits, which needs no data.
  limits              the 32767-character limit (-1 and ERANGE, the characters
                      up to the limit and the null stay), widths and precisions
                      beyond 32767 (also `*`, INT_MIN), snprintf with a null
                      pointer and n of 0, %.32765f, %.32767e, %.32767g: a
                      32768-byte buffer between guards.
  stack               run with a 2,048-byte stack: %.1000f of the smallest
                      subnormal, %.1074f, %.0f of the largest, %.1000e and others
                      (data/stack.dat) are converted correctly, and then again
                      with the stack below the caller filled with a pattern, to
                      measure how deep vsnprintf goes: at most 900 bytes above
                      its caller's frame (the test allows 920: the call's
                      arguments and return address). It prints the deepest.
  purity              sprintf with integer, string and character conversions
                      only, and no floating-point operation or libc16 call in
                      the program: `members` must show no helper.
  dblonly             sprintf of doubles built from their bits: no helper either.
  nolink, only_sp, only_sn, only_vsp, only_vsn
                      call one function each; `members` says what links (the
                      engine and that function, and libc16's errno).

Data files
----------

One record per line, five tab-separated fields (the format is in the header of
gen and of lib/driver.h):

  P   %+08.3f   d:400921F9F01B866E   +003.142   8

  mode (P, or N and a number for snprintf with that n), format, arguments
  (i:int l:long d:16-hex-digit-double s:string S:null pointer), the expected
  output and its length. Strings and the other fields use \xHH, \\, \t, \n
  and \r. The programs turn the arguments into 16-bit words, as a __cdecl call
  pushes them (docs/ABI.md): variadic functions are called with that many
  `unsigned` words, v functions with a va_list that points at the words, so a
  wrong stack layout of a double, a long or a far pointer is a failure.

  intdi intuo intx    int conversions d i / u o / x X, every flag combination (32),
                      widths none 1 8 30, precisions none 0 1 6 17 20 40, one
                      value from the list each, rotating, then every value
                      (powers of 2, 8, 10 and 16 and neighbours, 0, +-1, INT_MAX,
                      INT_MIN, ...) with three of the combinations
  intmisc             the h modifier
  longdi1,2 longuo1,2 longx1,2
                      the same with l (LONG_MAX, LONG_MIN, ULONG_MAX, ...)
  dblxe dblxf dblxg   e E / f / g G: the same cross product with doubles
  dblspec1,2          zeros, infinities, NaNs (sign bit set or not, payloads)
                      with every flag combination
  dblcore             about a hundred values (ties 0.5 1.5 2.5 0.125 ..., halves,
                      0.1, 1/3, 2/3, the largest, smallest normal and subnormal)
                      at eight formats
  dblpow              powers of ten and two over the whole range, each with its
                      neighbours (the next double each way)
  dblties             ties and halves at precisions 0 to 20 in e f g
  dblrand             a seeded pseudo-random set of bit patterns (any exponent,
                      and exponents near zero) at %.17g %.20g %.0f %e %f %g
  dblexact dblmisc    the examples in the spec text, at length; the L modifier
  misc                * widths and precisions (negative too), %%, %s and %c in
                      every shape, null pointers, every invalid specification
  snp                 snprintf and vsnprintf with n of 0, 1, 2, 3, length-2,
                      length-1, length, length+1, length+2 and length+9
  fast1..3, stack     see above

Oracle
------

The expected output comes from a model of the spec written in the generator,
which gets the exact digits from Math::BigInt arithmetic on the binary value
(round half even on the exact value), and not from Perl's sprintf. With
--verify, every valid line that has no infinity, NaN or null pointer is
also formatted by Perl's own sprintf over the host's C library, and compared:
on this host (macOS) 31,628 lines agree, and one differs: '%.0f' of
0x3FE0000000000001 (0.5 plus one unit in the last place), which Perl
rounds down to 0 and the spec (and the exact value) says is 1. The spec is
followed where Perl or the C library differs from it:

  - inf and nan (Perl prints Inf, NaN; the spec: inf, nan, -nan with the sign
    bit, INF and NAN for E and G, padded with spaces, never zeros);
  - a null %s argument (Perl: empty; the spec: (null));
  - the 0 flag with c, s and %% (no effect, as the spec says);
  - invalid and unsupported specifications (%n %p %a %lld %hh ... are copied as
    written, from the % through the offending character, no argument consumed);
  - %p is not supported; h is the same as no modifier.

Not in the corpus, because the spec leaves them open (see the report of the
session that wrote these): %lf %hf %Ld %hs %Ls %hc (a length modifier with a
conversion it is not listed for), a null %s with a precision below 6, and
errno after ordinary success.

Size and time
-------------

About 29,000 records, 1.6 MB. Most of the time is the exact conversions
(dblxe dblxg dblxf dblpow dblcore dblrand dblexact), the rest is cheap.
Under DOSBox-X's default speed (about 3 million cycles a second: run-dos
sets no cycles) an integer record costs about 4,000 cycles (1.5 ms) with a
trivial implementation and a double 10 to 100 times that, so allow several
minutes per model, and RUN_DOS_TIMEOUT (tests/run-dos, default 600
seconds for the whole suite) may need raising; limits and dblexact have the
longest records. Splitting is by data file: delete cases to run fewer.
