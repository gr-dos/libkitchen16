# Spec: `strtod` and `atof`

**Members:** `strtod`, `atof` (`atof` calls `strtod`; a program that calls neither links neither)  
**Header:** `<kitchen16.h>`  
**Tests:** `tests/conv` (run by `tests/run-dos`)

This is the behavioural spec an implementation is written from (DESIGN.md, decision 6). It does not describe any existing implementation.

## Sources

C89 7.10.1.1 (`atof`) and 7.10.1.4 (`strtod`); C99 7.20.1.3 for the `inf`, `infinity` and `nan` forms; IEEE 754-1985 (binary64, round to nearest even). The host's `strtod` is the test oracle only.

## Interface

    double __cdecl strtod(const char *nptr, char **endptr);
    double __cdecl atof(const char *nptr);

Both are `__cdecl` (`docs/ABI.md`). In the compact and large models the pointers are far, as in every libkitchen16 function.

## `strtod`

Converts the initial portion of the string `nptr` to a `double`.

### Syntax accepted

1. Optional white space, as `isspace` of the C locale gives it (space, `\t`, `\n`, `\v`, `\f`, `\r`).
2. An optional `+` or `-`.
3. Then one of:
   - a **decimal number**: a non-empty sequence of digits optionally containing one `.`, with at least one digit in all, then an optional exponent: `e` or `E`, an optional sign, and a non-empty sequence of digits. An exponent marker not followed by digits (`1e`, `1e+`) is not part of the number, which ends before the `e`;
   - `inf` or `infinity`, in any mix of upper and lower case;
   - `nan`, in any case, optionally followed by `(` and a sequence of letters, digits and underscores and a `)`. If the `(` does not begin such a sequence (`nan(`, `nan(a b)`), the number ends after the `nan`.
4. **Hexadecimal floating constants (`0x1.8p3`) are not accepted**: `0x10` is the number `0` followed by `x10`. (C99's, and not C89's.)

### Result

- The value is **correctly rounded**: of the two `double`s that surround the exact value of the decimal number, the nearer one; ties to even. This holds for every input, however many digits it has (the digits after the 800th matter only through whether any of them is non-zero, which can break a tie), for exponents as large as `±99999`, and across the subnormal range (gradual underflow, rounding to the nearest multiple of `2^-1074`). It must not depend on `double` arithmetic: it is exact integer work.
- A leading `-` negates the result, also for zero (`-0` gives negative zero).
- `inf` and `infinity` give an infinity of the sign given; `nan` gives a quiet NaN with the sign given.
- If the value is too large (rounds to `2^1024` or more): the result is `HUGE_VAL` (an infinity) with the sign, and `errno` is set to `ERANGE`.
- If the value is non-zero but rounds to zero (it is closer to zero than to the smallest subnormal, `2^-1074`, or exactly halfway, `2^-1075`, which rounds to even, that is to zero): the result is a zero of the sign given, and `errno` is set to `ERANGE`. A result that is subnormal and non-zero sets `errno` to `ERANGE` only if it is inexact; when it is exact the result is not an underflow and `errno` is left alone.
- If no conversion can be made (the string has no number after the optional white space and sign): the result is `0.0`, and `*endptr` is set to `nptr`, so the whole string, white space included, is "not converted". `errno` is not set (the C89 standard does not require `EINVAL`).
- `endptr` may be a null pointer. Otherwise `*endptr` points at the first character after the longest initial portion accepted by the syntax above.
- `errno` is never cleared: it is only set, to `ERANGE`, as above.

## `atof`

`atof(s)` is `strtod(s, (char **)0)`, including its use of `errno`.

## Size and stack

- The members use no static storage that grows with the input. The exact conversion works on a bounded buffer of digits and a big integer of a size that depends only on the format: about 1,100 bits for the largest and smallest values. A string of any length converts with the same bounded memory.
- Stack use is at most 700 bytes for any input, measured and recorded in `docs/DESIGN.md`.
- A program that calls neither function links none of these members.

## Must

- Not link anything from Open Watcom's runtime; link from libc16 only `errno` (and its own startup).
- Not call any `-fpc` helper: the result is assembled from its bits. (A program that calls `strtod` and does no floating-point arithmetic itself links no helper; `tests/conv/members` checks it.)
- Use 8086 instructions only.

## Tests

`tests/conv` runs in every model. Each test line has a string (written to the data file with escapes so that it can carry any byte) and the expected result's bits, `endptr` offset and `errno` flag, generated on the host with its `strtod` (`tests/conv/gen`). The corpus has: every syntax rule above, signs, white space, `inf`/`nan` spellings, a power of ten and of two across the whole range, halfway cases of every binade (a decimal string exactly halfway between two doubles, and the same with one more digit that is a 1 and a 0), long digit strings (800 digits and more, with a non-zero digit far out), the largest finite value and just above it, the smallest subnormal and just below half of it, the smallest normal and the values around it, and a large pseudo-random set of decimal strings of 1 to 25 digits with random exponents. A round-trip program prints with `sprintf("%.17g")` and reads back, for a random set of bit patterns.
