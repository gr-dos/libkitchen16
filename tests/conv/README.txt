tests/conv: tests of strtod and atof (docs/spec/conv.md)

Files
  cases         run by tests/run-dos: NAME STATUS PROGRAM [ARGUMENTS]; every case
                prints "ok" (NAME.expect) and exits 0
  members       the library members each program links (tests/check-members)
  rules.mk      links the helper objects of lib/, gives longstr a 1,024 byte stack
  gen           Perl generator of data/*.DAT (committed); `tests/conv/gen --check`
                also cross-checks the expected values (see below)
  data/*.DAT    the corpus, copied by run-dos to the DOS directory
  lib/data.c    reads and parses a data file, compares, reports (no floating point)
  lib/hex.c     prints 16 bits as 4 hex digits
  conv.c        strtod against a data file: bits, endptr offset, errno == ERANGE
  errno.c       errno is never cleared (a sentinel, or ERANGE, is set before each
                call) and is ERANGE exactly when expected; also for atof and for a
                null endptr
  atof.c        atof(s) has the bits of strtod(s, 0) and of the data line, same errno
  purity.c      calls strtod and does no floating-point operation: it must link no
                -fpc helper (members)
  longstr.c     strings of 5,000 to 6,000 digits built at run time (ties, zero
                runs, 9 runs, white space, ...) with a stack of 1,024 bytes
  roundtrp.c    sprintf("%.17g") then strtod for 1,200 pseudo-random finite bit
                patterns and special values: the same bits (needs fmt's sprintf)

Data files (one program run each, so a failure names the corpus)
  SYNTAX   every syntax rule: white space (the six of isspace, and bytes that are
           not), signs, number shapes, exponent without digits, hex constants (not
           accepted), inf/infinity/nan spellings, nan(...) forms, early ends
  POW10    powers of ten 1e-330 .. 1e310 with the numbers just below and above, and
           written out in full
  POW2     exact decimal expansions of powers of two, 2^-1076 .. 2^1023 (every
           fourth, and all near the ends and near 1)
  HALF     the exact midpoint between two neighbouring doubles, in every binade
           (every bit length of subnormal, every normal binade, random odd and
           even mantissas, the overflow tie), written in plain or e notation
  HALF2    the midpoint with a 1 appended, with seven 0s appended, one decimal
           unit below, and below with 9s appended, for every sixth binade
  EDGE     largest finite and just above, smallest subnormal and just below half of
           it, the exact 2^-1075 tie, smallest normal and neighbours, 1 +- half
           an ulp, 2^53 neighbours, the values around them in full decimal
  LONGDIG  800 to 1,200 digit strings: zero runs, ties and near ties extended to
           798..802 and 1,000 digits and with a non-zero digit 850 places out,
           long random numbers
  BIGEXP   exponents beyond +-99999 (1e999999999: must not wrap around) and the
           exponent 99999, long mantissas with offsetting exponents
  RANDOM   1,800 pseudo-random decimal strings of 1 to 25 digits, a point at a
           random place, exponents -335 .. 324, signs, leading white space,
           trailing junk (fixed seed)

Line format: BITS OFFSET ERR STRING, single spaces.
  BITS    16 upper case hex digits, the expected bits of the result; or nan+ or
          nan- (only "is a NaN" and its sign are compared)
  OFFSET  the expected endptr - nptr in decimal (0: no conversion)
  ERR     R if errno must be ERANGE afterwards, - if not
  STRING  to the end of the line: every byte except ! .. ~ other than backslash is
          written \xHH (lower case hex); the backslash is \x5c. The empty string
          is nothing after the third space. Lines are at most 2,000 characters.

Generation
  tests/conv/gen            writes data/*.DAT (about 2 MB)
  tests/conv/gen --check    the same, and compares every decimal result with Perl's
                            own conversion (the host C library) and a sample of a
                            fifth with an exact check on the neighbouring doubles
                            (Math::BigRat); exit status 1 on a difference
The expected values are computed by the generator from the spec's rules with exact
integer arithmetic; the host's strtod is the cross-check, not the source. Rules
taken from the spec where the host differs or is silent: no hex floats; endptr by
the spec's syntax; ERANGE for a non-zero value that rounds to zero, and for an
inexact subnormal, but not for an exact subnormal and not for a result that
rounds up to the smallest normal number.
