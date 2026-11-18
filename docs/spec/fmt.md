# Spec: formatted output to memory (`fmt`)

**Members:** one for the conversion engine and one for each of the four functions (a program that calls `sprintf` links the engine and `sprintf`, not `snprintf`); the engine may be split further, as long as the rule in "Linking" holds  
**Header:** `<kitchen16.h>`  
**Tests:** `tests/fmt` (run by `tests/run-dos`)

This is the behavioural spec an implementation is written from (DESIGN.md, decision 6). It does not describe any existing implementation.

## Sources

C89 7.9.6.1 (`fprintf`'s conversions), 7.9.6.5 (`sprintf`) and 7.9.6.9 (`vsprintf`); C99 7.19.6.5 (`snprintf`) and 7.19.6.12 (`vsnprintf`) for the bounded forms and their return value; IEEE 754-1985 (binary64); the `<stdarg.h>` of libc16 (`include/stdarg.h`). The host's C library is the test oracle only.

## Interface

    int __cdecl vsnprintf(char *s, size_t n, const char *format, va_list ap);
    int __cdecl snprintf(char *s, size_t n, const char *format, ...);
    int __cdecl vsprintf(char *s, const char *format, va_list ap);
    int __cdecl sprintf(char *s, const char *format, ...);

All are `__cdecl` (`docs/ABI.md`). `va_list` is libc16's. A `double` argument of a variadic call is 8 bytes on the stack, as a `__cdecl` argument is (`docs/ABI.md`, "Floating-point arguments and results"), so `va_arg(ap, double)` fetches it; a `float` is promoted to `double` by the caller.

## Behaviour

`vsnprintf` writes the formatted output to `s`, at most `n - 1` characters followed by a null character; if `n` is 0 nothing is written (and `s` may be a null pointer). It returns the number of characters that **would have been written** had `n` been large enough, not counting the null character. `snprintf` is `vsnprintf` with the arguments after `format`. `vsprintf` and `sprintf` write all of the output, as C89 defines them, followed by a null character, and return the number of characters written, not counting it.

`size_t` and `int` are 16 bits. If the number of characters of the output would exceed 32767, the functions return -1 and set `errno` to `ERANGE`; the characters already written, up to the limit of `n`, stay in the buffer and the string is null-terminated as above when `n` is not 0. For `vsprintf` and `sprintf`, which have no `n`, the function stops writing at 32767 characters and writes the null character after them. No function writes outside `s[0 .. n-1]`.

## The format

The format is a string of ordinary characters, copied to the output, and conversion specifications. A specification is, in order:

1. `%`;
2. zero or more **flags**: `-` (left-justify), `+` (always a sign), space (a space instead of a plus sign), `#` (alternate form), `0` (pad with zeros);
3. an optional **width**: a decimal number, or `*`, which takes an `int` argument; a negative `*` width is the `-` flag with its absolute value. A width, or a precision, of more than 32767 (written in digits, or the absolute value of `INT_MIN`) is taken as 32767;
4. an optional **precision**: `.` followed by a decimal number, or by `*` (an `int` argument; a negative one is as if there were no precision), or by nothing (a precision of 0);
5. an optional **length modifier**: `h` (the argument is an `int` or `unsigned` that was a `short`; on this target the same as none), `l` (the argument is a `long` or `unsigned long`), `L` (for a floating conversion; the argument is a `double`, since `long double` is `double` on this target);
6. a **conversion specifier**.

The argument for `*` comes before the argument converted, width first.

### Conversions

| Specifier | Argument | Output |
|---|---|---|
| `d`, `i` | `int` (`long` with `l`) | signed decimal |
| `u` | `unsigned` (`unsigned long` with `l`) | unsigned decimal |
| `o` | the same | unsigned octal |
| `x`, `X` | the same | unsigned hexadecimal, lower case (`abcdef`) or upper case (`ABCDEF`) |
| `c` | `int` | the character, converted to `unsigned char` |
| `s` | `const char *` (far in the compact and large models) | the characters of the string up to but not including its null character, or up to the precision if there is one. A null pointer prints as the six characters `(null)`, and width, precision and `-` apply to them as to any string: `%.3s` gives `(nu`, `%8s` gives two spaces and `(null)`, `%-8s` gives `(null)` and two spaces, `%.0s` gives nothing (C89 leaves a null pointer undefined; see "Choices beyond the C standard") |
| `e`, `E` | `double` | `[-]d.ddde±dd`, `E` for `E` |
| `f` | `double` | `[-]ddd.ddd` |
| `g`, `G` | `double` | `e` or `f` style, as below |
| `%` | none | a `%`. The whole specification must be `%%`: `%5%`, `%-%`, `%.1%`, `%l%` and `% %` are invalid specifications, copied as written (C89 7.9.6.1 says only that the complete specification shall be `%%`) |

An integer conversion with a precision gives at least that many digits, with leading zeros, and the `0` flag is then ignored; the default precision is 1; a zero value with precision 0 gives no digits (but `#` with `o` gives `0`). The sign of `d` and `i` is `-` for a negative value, otherwise `+` with the `+` flag, otherwise a space with the space flag. `#` gives `o` a leading `0` if the first digit isn't already `0` (by precision), and gives a non-zero value in `x` and `X` the prefix `0x` or `0X`.

**Width and padding.** If the output is shorter than the width it is padded on the left with spaces, or on the right with the `-` flag, or on the left with zeros after the sign and prefix if the `0` flag is given, there is no `-` flag, and (for integer conversions) no precision. `0` has no effect on `c`, `s` and `%`, nor on `inf` and `nan`, which are padded with spaces.

**Floating conversions.** The precision is the number of digits after the decimal point for `e`, `E` and `f` (default 6), and the number of significant digits for `g` and `G` (default 6; a precision of 0 is 1). The default and every explicit precision up to 32767 must work.

- **`e`, `E`:** one digit before the decimal point, then the point and *precision* digits, then the exponent: `e` or `E`, a sign, and at least two digits (more if needed, up to three for a `double`). With precision 0 there is no point, unless `#`. A zero value has exponent `+00`.
- **`f`:** at least one digit before the point; with precision 0 there is no point, unless `#`.
- **`g`, `G`:** let *P* be the precision, taken as 1 if 0, and *X* the exponent that `e` style would have after rounding to *P* significant digits. If *P* > *X* ≥ -4 the style is `f` with precision *P* - 1 - *X*, otherwise `e` with precision *P* - 1. Unless `#` is given, trailing zeros are removed from the fractional part, and the point too if no fraction digits remain. With `#`, trailing zeros and the point are kept.
- A negative value (including `-0`) prints a `-`; otherwise `+` or a space as for `d`.
- **Infinities and NaNs** print `inf` or `nan` (`INF`, `NAN` for `E` and `G`; `f` and `e` and `g` give lower case), after the sign `-` if the sign bit is set (so `-nan` for a NaN with it, as C99's `printf` does), with the `+` and space flags as for numbers. They are padded with spaces, never zeros.

### Exactness

- The conversions `e`, `E`, `f`, `g` and `G` give the **exactly rounded decimal** of the binary value: the digits are those of the exact binary value, rounded to the number of digits asked for, ties (when the exact value is exactly halfway between two decimals) to even. For example `%.0f` of `2.5` is `2` and of `3.5` is `4`; `%.20g` of `0.1` is `0.10000000000000000555`; `%.20g` of `1/3.0` is `0.33333333333333331483`; `%.30f` of `1.0` is `1` followed by the point and 30 zeros; `%.1000f` of `5e-324` is the exact expansion (751 significant digits), then zeros.
- This holds at any precision, for every value: normal, subnormal, the largest (`%.0f` of `1.7976931348623157e308` is its exact 309-digit decimal expansion) and the smallest.
- It must not depend on `double` arithmetic (that is, on the `-fpc` helpers being exact or linked): the digits are made by exact integer work on the bits.
- **Fast path.** A finite `double` that is an integer of magnitude below `2^53`, formatted with `f`, or with `g` or `G` when the precision allows all its digits, is formatted from its integer value, without the big-integer conversion. The output is identical to the general path's for every precision, flag and width, which `tests/fmt` checks.

### Invalid and unsupported specifications

`%n`, `%p`, `%a`, `%A`, `%ls`, `%lc`, `%lld` and every other conversion specifier or length modifier not listed are **not supported**. A specification that is not valid is copied to the output as written, from the `%` through the offending character, and consumes no argument; formatting continues after it. (So `%n` gives the two characters `%n`, and `%lld` gives `%ll` and then, since `d` is an ordinary character at that point, `d`.) A format that ends inside a specification copies what it has. A specification is invalid if its `*` is followed by digits, or if a modifier follows another modifier.

### Choices beyond the C standard

Where C89 7.9.6.1 leaves the behaviour undefined, or an implementation to define, this spec fixes one, and `tests/fmt` checks it (`tests/fmt/gen` writes the expected output from this text; the host's `sprintf` agrees except where `tests/fmt/README.txt` lists a difference). A program that must also run with another C library must not rely on these:

| Case | C89 | Here | Portability |
|---|---|---|---|
| `%s` with a null pointer | undefined | prints `(null)` as a string: width, `-` and precision apply (`%.3s` gives `(nu`) | other C libraries differ (some print `(null)` only without a precision, some crash); do not pass a null pointer |
| `%5%`, `%-%`, `%.1%`, `%l%` | undefined (the specification shall be `%%`) | invalid: copied as written, no argument consumed, formatting continues after the `%` | other C libraries differ in what they print; write `%%` |
| `h`, `l` or `L` on a conversion they do not apply to (`%lf`, `%hf`, `%Ld`, `%hs`, `%hc`) | undefined | accepted and ignored, as if absent; `ls` and `lc` (C99 wide) are the exceptions: invalid, copied as written | |
| `%n`, `%p`, `%a`, `%A`, `%lld`, `%zu`, `%jd`, `%td` | `%n` and `%p` defined; the rest C99 | invalid, copied as written, no argument consumed | a program must not use them |
| Output over 32767 characters | (an `int` count) | -1 and `errno` `ERANGE`; the 32768th character is never delivered, so at most 32767 characters and the null stay in the buffer | |
| `-nan` | not defined (C89 has no NaN) | a NaN with the sign bit set prints `-nan`, as C99's `printf` does in common implementations | a program that compares output text with another library's must allow `nan` |

## Memory and stack

- The functions use no static storage (they are reentrant, and need no initialisation), and write only into `s` and the stack.
- The exact conversion of a `double` uses a bounded work area (a big integer of about 1,100 bits and a digit buffer); the whole of `vsnprintf` uses at most 900 bytes of stack above its caller's frame, with any precision, measured and recorded in `docs/DESIGN.md`. The output is produced as it is converted: a precision of 32767 does not need a buffer of that size.
- `tests/fmt` runs a case with a 2,048-byte stack and `%.1000f` of the smallest subnormal.

## Linking

- A program that calls only `sprintf` with integer and string conversions links the engine, which may include the floating-point conversion; a program that calls nothing in this spec links none of it.
- No member links anything from Open Watcom's runtime. From libc16 a member may link `errno` and nothing else; it never calls an `fpc` helper (`tests/fmt/members` checks the members linked by programs with and without `double` conversions).
- Use 8086 instructions only.

## Tests

`tests/fmt` runs in every model. Each line of the data file has a format, a list of arguments (a `double` as its 16 hex digits, so that no string-to-double conversion is needed), and the expected output, generated on the host by Perl's `sprintf` over its C library (`tests/fmt/gen`), which is the oracle only. The corpus is the cross product of every conversion with flags (all 32 combinations), widths (none, 1, 8, 30), precisions (none, 0, 1, 6, 17, 20, 40), and a list of values: for integers, zero, one, minus one, `INT_MAX`, `INT_MIN`, `LONG_MAX`, `LONG_MIN`, `ULONG_MAX`, and powers of 2, 8, 10 and 16 and their neighbours; for `double`s, the zeros, infinities, NaN, powers of ten and two (each with both neighbours) over the whole range, ties (`0.5`, `1.5`, `2.5`, 0.125, …), halves of every kind, `0.1`, `1/3`, `2/3`, the largest, the smallest normal, the smallest subnormal, and a large pseudo-random set of bit patterns at `%.17g`, `%.20g`, `%.0f`, `%e`, `%f` and `%g`; also `*` widths and precisions, `%%`, the invalid specifications above, and `snprintf` with `n` of 0, 1, the output's length, one less and one more. The integer fast path is checked by running every integral value of a corpus through the same conversions and precisions as the general path.
