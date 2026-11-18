# Spec: floating-point helpers (`-fpc`)

**Members:** one per symbol below, or one per closely related group, so that a program links only the operations it uses  
**Tests:** `tests/fpc` (run by `tests/run-dos`), `tests/check-abi`

This is the behavioural spec an implementation is written from (DESIGN.md, decision 6). It does not describe any existing implementation.

## Sources

- Which operation calls which symbol, and each symbol's register convention: call-site experiments, compiling one-line C functions with the consumer flags of `docs/ABI.md` and `-fpc`, and reading the `EXTRN` records and the call sites in the disassembly (`wdis -a`) of **our own objects** (`docs/ABI.md`, "Compiler support routines").
- IEEE 754-1985 (binary64 and binary32 formats, round to nearest even, subnormals, signed zeros, infinities, NaNs).
- C89 3.2.1.3 (a floating value converts to an integer by truncating toward zero) and 3.2.1.4 (`double` to `float` and back).

No Open Watcom runtime source or header is read, and none of its libraries is disassembled (decision 6).

## What `-fpc` is

`wcc -fpc` compiles floating-point operations as calls to helper routines instead of x87 instructions or an emulator. libkitchen16 provides the helpers. A program compiled with `-fpc` and linked with them needs no FPU and no emulator.

## Formats

- **`double`** is IEEE 754 binary64: sign (bit 63), 11-bit exponent biased by 1023 (bits 62 to 52), 52-bit fraction (bits 51 to 0). It is stored low word first, so bits 15 to 0 are at the lowest address.
- **`float`** is IEEE 754 binary32: sign (bit 31), 8-bit exponent biased by 127, 23-bit fraction.
- **In registers** (`docs/ABI.md`, "Floating-point helpers"): a `double` is `AX:BX:CX:DX`, `AX` holding bits 63 to 48 and `DX` bits 15 to 0; a `float` or a 32-bit integer is `DX:AX`, `DX` holding the high word.

## Symbols

`a` is the left operand, `b` the right. A "memory operand" is 8 bytes, low word first, at `SS:SI`; the `E` forms take it at `ES:SI` instead (the compiler uses them in the compact and large models, when `b` is reached through a far pointer).

| Symbol | Operation | In | Out |
|---|---|---|---|
| `__FDA`, `__EDA` | `a + b` | `a` = `AX:BX:CX:DX`, `b` = memory operand | `AX:BX:CX:DX` |
| `__FDS`, `__EDS` | `a - b` | the same | `AX:BX:CX:DX` |
| `__FDM`, `__EDM` | `a * b` | the same | `AX:BX:CX:DX` |
| `__FDD`, `__EDD` | `a / b` | the same | `AX:BX:CX:DX` |
| `__FDC`, `__EDC` | compare `a` with `b` | the same | the flags (below) |
| `__FDN` | `-a` | `a` = `AX:BX:CX:DX` | `AX:BX:CX:DX` |
| `__FDI4` | `(long)a`; also `(int)a`, taken from `AX` | `AX:BX:CX:DX` | `DX:AX` |
| `__FDU4` | `(unsigned long)a`; also `(unsigned)a` | `AX:BX:CX:DX` | `DX:AX` |
| `__I4FD` | `(double)l` for `long l`; also `int`, widened first | `DX:AX` | `AX:BX:CX:DX` |
| `__U4FD` | `(double)u` for `unsigned long u`; also `unsigned` | `DX:AX` | `AX:BX:CX:DX` |
| `__FDFS` | `(float)a` | `AX:BX:CX:DX` | `DX:AX` |
| `__FSFD` | `(double)f` for `float f` | `DX:AX` | `AX:BX:CX:DX` |

The `E` forms are called only in the compact and large models; the small model uses the `F` forms alone (the symbols the compiler emits are in `tests/abi/fpc.c`, which `tests/check-abi` checks). Every `E` form gives the result of its `F` form for the same operands.

The helpers for `long long` conversions (`__FDI8`, `__I8FD`, …) and for `float` arithmetic (`__FSA` and the other `__FS*`) are not provided. A program that needs one fails to link.

## Memory models

Each member is assembled or compiled for the small, compact and large models. Helpers are called near in the small and compact models and far in the large model, returning with `ret` or `retf`. The memory operand is in `DGROUP`: at `SS:SI`, not `DS:SI`, since `DS` may hold any segment on entry in the compact and large models; the `E` forms use `ES:SI`, wherever it points. Registers, operands and results are otherwise the same in every model.

## Registers

- A helper changes `AX`, `BX`, `CX`, `DX` and the flags, and nothing else. It preserves `SI`, `DI`, `BP`, `DS`, `ES` and `SS` (the `E` forms do not change `ES` or `SI`), and leaves the direction flag as it found it.
- `__FDC` and `__EDC` change only the flags: they preserve `AX`, `BX`, `CX` and `DX` too.
- A helper never writes its memory operand.
- A helper uses no more than 64 bytes of stack and calls nothing outside libkitchen16's helpers. `tests/stack` measures each helper on every pair of the values it tries, with the function that calls it taken off: at most 44 bytes small, 48 compact and 54 large (`__FDM`, the deepest, its return address included).

## Arithmetic: `+ - * /`, `-`

IEEE 754 binary64:

- The result is the exact mathematical result rounded to the nearest representable `double`, ties to even. This holds for every operation, including results in the subnormal range (gradual underflow, no flush to zero) and results that overflow (an infinity of the right sign).
- Signed zeros follow IEEE 754: `+0 + -0` is `+0`; `x - x` is `+0`; `-0 + -0` is `-0`; the sign of a product or quotient is the exclusive or of the signs of the operands, also for zeros and infinities.
- Infinities: `inf + inf` is `inf`, `inf - inf` is a NaN, `inf * 0` is a NaN, `inf / inf` and `0 / 0` are NaNs, `x / 0` for finite non-zero `x` is an infinity of the right sign, `x / inf` is a zero, `inf * x` is an infinity for non-zero `x`.
- NaNs: an invalid operation (above) gives the default NaN, `0x7FF8000000000000` (positive, quiet, empty payload). When an operand is a NaN the result is that operand with its quiet bit set (`a` first if both are NaNs). A NaN's payload and sign are otherwise not significant to the tests.
- `__FDN` flips the sign bit of every value, including zeros, infinities and NaNs.
- There are no exception flags, no traps, no rounding modes and no `<fenv.h>`. A helper never raises an interrupt: not for division by zero, not for overflow, not for an invalid operation.

## Comparison: `__FDC`, `__EDC`

The result is in the flags, as if by `cmp x, y` with these operands, which make both the signed and the unsigned conditional jumps correct:

| `a` against `b` | Flags as for `cmp` of | So |
|---|---|---|
| `a < b` | `0` with `1` | `jl`, `jb` taken; `je` not |
| `a == b` | `0` with `0` | `je`, `jge`, `jle`, `jae`, `jbe` taken |
| `a > b` | `1` with `0` | `jg`, `ja` taken; `je` not |
| unordered (either is a NaN) | as `a > b` | |

`+0` and `-0` are equal. `-inf` is less than every finite value, `+inf` greater. A comparison with a NaN is therefore *greater*: `a != b`, `a > b` and `a >= b` are true, `a == b`, `a < b` and `a <= b` false. C and IEEE 754 want `a > b` and `a >= b` false too. The next section shows why no choice of flags can give that, and the section after it a related limit that is the compiler's.

### What the compiler does with the flags

Found by compiling `tests/abi/fpccmp.c` with the consumer flags in each model and reading our own objects (`tests/check-abi`), and confirmed by running every operator in every shape (`tests/fpc/nancmp.c`):

- Each of `==`, `!=`, `<`, `<=`, `>`, `>=` is one call of `__FDC` (or `__EDC`, with a right operand through a far pointer) followed by a branch on **one signed condition**. `je jne jl jge jle jg` are the only conditions the compiler branches on after these calls; it never uses the unsigned (`ja jb jae jbe`), carry, sign, overflow or parity conditions.
- The operands are passed in the order written (`b < a` calls the helper with `b` first; the compiler does not turn it into `a > b`), and the code layout may invert the branch (`jl` or `jge` for the same test), but the condition each operator tests is fixed:

| Operator | True when | Condition |
|---|---|---|
| `==` | `ZF = 1` | `je` |
| `!=` | `ZF = 0` | `jne` |
| `<` | `SF ≠ OF` | `jl` |
| `>=` | `SF = OF` | `jge` |
| `<=` | `ZF = 1` or `SF ≠ OF` | `jle` |
| `>` | `ZF = 0` and `SF = OF` | `jg` |

- **`<` and `>=` are complements, and so are `<=` and `>`**, because they test the same flags. Whatever the helper returns for an unordered pair, at least one of each pair is true, so at least two of the four relational operators are true for a NaN. Every choice of `ZF` and `SF ≠ OF` for the unordered pair, against what IEEE 754 requires (`==` false, `!=` true, the four relational operators false):

| Unordered pair reported as | Operators that are wrong |
|---|---|
| greater (`ZF = 0`, `SF = OF`): the choice here | `>`, `>=` |
| less (`ZF = 0`, `SF ≠ OF`) | `<`, `<=` |
| equal (`ZF = 1`, `SF = OF`) | `==`, `!=`, `<=`, `>=` |
| `ZF = 1`, `SF ≠ OF` | `==`, `!=`, `<`, `<=` |

  So the best a helper can do is two wrong operators, and the choice is between "greater" and "less". *Greater* keeps `while (x < limit)` loops finite for a NaN, makes `x >= lo && x < hi` reject it (`>=` true, `<` false), keeps `==` false and `!=` true, and matches the sort order that puts NaNs last; *less* would make `x < limit` true for a NaN, an endless loop. That decides it. The limitation is the compiler's convention (one flags result, signed conditions), so it cannot be removed by the helper, and the ABI is not changed to hide it.
- No compiler option is known to change this: the operators were compiled with `-os` (the consumer flags), and the zero shortcut below is the same with `-od`, `-ot`, `-ox`, `-on` and `-oe`. `-fpi` and `-fpi87` compare with x87 instructions for an emulator or an 8087, which libkitchen16 does not provide, so they are not an option.

### Comparisons the compiler does not send to a helper

A comparison of a `double` with a **literal `0.0`**, `!x`, `if (x)`, `while (x)`, `x && y` and `x ? a : b` call no helper in any model: `wcc -fpc` tests the **high 16 bits** of the operand (the sign, the exponent and the top four bits of the fraction; `AX`) and nothing else (`tests/abi/fpccmp.c`, `tests/check-abi`). `*p == 0.0` compares the high word in memory with zero. It is the same under every optimisation option tried (`-od`, `-ot`, `-ox`, `-on`, `-oe`). Compared with IEEE 754 this makes:

| Operand | Compiled as | IEEE 754 |
|---|---|---|
| `-0.0` | non-zero, and *less than* zero (`x == 0.0` and `!x` false, `x != 0.0`, `x < 0.0` and `if (x)` true) | equal to zero |
| a subnormal whose high 16 bits are clear (magnitude below about `1.4e-309`) | zero | not zero |
| a NaN | non-zero; positive or negative by its sign bit (`x > 0.0` true for `+NaN`, `x < 0.0` for `-NaN`) | unordered |
| `+inf`, `-inf`, every other value | by the high word's sign and zero-ness, which is right | |

These are the compiler's choices, not the helpers' (there is no call to change), and `tests/fpc/nancmp.c` records them so that a compiler change shows. A comparison with any other constant (`x == 1.0`, `x < 100.0`) and with a zero that is not a literal calls the helper and is right.

### What a program that needs IEEE results does

A program that may meet a NaN, a `-0.0` or a tiny subnormal, and cares, writes its comparisons so that the compiler does not take the shortcut and the unordered case is tested. These idioms are tested (`tests/fpc/nancmp.c`):

    static volatile double zero;    /* a zero that is not a literal: x == zero calls __FDC */
    if (x != zero) ...              /* instead of if (x) and x != 0.0; -0.0 is zero, NaN is not */

    static int isnan_(double x)     /* by the bits: no helper, no FPU */
    {
        unsigned short *w = (unsigned short *)&x;      /* w[3] holds bits 63..48 */
        return (w[3] & 0x7FF0) == 0x7FF0 && ((w[3] & 0x000F) | w[2] | w[1] | w[0]);
    }
    ordered_lt = !isnan_(a) && !isnan_(b) && a < b;    /* ordered_ge likewise: >= with the two tests */

A program that does not use NaNs needs nothing (an ordered comparison is exact), and it can leave a literal-zero test as it is if it never makes `-0.0` or a subnormal below `1.4e-309`; `perl16` should check its truth test of a number and its comparisons with `0.0` for this. libkitchen16 provides no `isnan` or wrapper macros: the idioms are four lines, and the ABI of the helpers is not changed for them.

## Conversions to integers

`__FDI4`, `__FDU4` truncate toward zero. Out of range is undefined in C; here it is defined, so that results are the same on every run:

| `a` | `__FDI4` result | `__FDU4` result |
|---|---|---|
| NaN | `0` | `0` |
| truncates to a value in `-2^31` to `2^31 - 1` | that value | that value's two's complement, as an unsigned 32-bit number (so `-1.0` gives `0xFFFFFFFF`) |
| above `2^31 - 1`, up to `2^32 - 1` | `LONG_MAX` (`0x7FFFFFFF`) | that value |
| `2^32` or more, or `+inf` | `LONG_MAX` | `ULONG_MAX` (`0xFFFFFFFF`) |
| below `-2^31`, or `-inf` | `LONG_MIN` (`0x80000000`) | `0x80000000` |

`(int)a` and `(unsigned)a` use the low word of these results (the compiler calls the same helper and reads `AX`), which for values within the 16-bit range is the value itself.

## Conversions from integers

`__I4FD` and `__U4FD` give the exact value when it is representable, and otherwise do not occur: every 32-bit integer is exactly representable in binary64. Zero converts to `+0`.

## `float`

- `__FDFS` rounds to the nearest binary32, ties to even, including into the subnormal range (below `2^-126`) and to zero. Magnitudes that round to `2^128` or more give an infinity of the right sign; a NaN stays a NaN (quiet; the high bits of the payload carry over); signed zeros keep their sign.
- `__FSFD` is exact: every binary32 value is a binary64 value. Infinities, zeros (with sign) and NaNs (quiet; payload carried to the high bits) convert as such; binary32 subnormals become normal binary64 numbers.

## Must

- Give the results above for every operand value, including all subnormal, zero, infinite and NaN combinations.
- Follow the register convention exactly: operands where the compiler puts them, results where it reads them, every other register preserved.
- Use 8086 instructions only (`scripts/check-8086`), and keep each operation small: they are linked into programs that care about size. A helper may be written in assembly (`src/model.inc` selects the model) or in C that avoids `long` multiplication and division, `long long`, and anything else that links a helper from libc16: **the members of `fpc` link nothing from libc16** (`tests/fpc/members` checks it) and nothing from Open Watcom.
- Be independently linkable: a program that only adds links `__FDA` and what it shares, not `__FDD`.
- Record each convention in `docs/ABI.md`, "Floating-point helpers", with the experiment it came from.

## Tests

`tests/fpc/nancmp.c` runs every comparison operator in the shapes the compiler has (a value, a branch, through pointers) on every pair of 20 values, with and without NaNs, and records the literal-zero behaviour and the idioms above; `tests/abi/fpccmp.c` and `tests/check-abi` check which helper and which conditions the compiler uses (the signed ones only), and that a literal zero calls no helper.

`tests/fpc` runs in every model. The programs are compiled with `-fpc`, so the compiler's own call sites exercise each helper; each reads its operands as bit patterns from a data file generated on the host (`tests/fpc/gen`), computes with C operators, and compares the result's bits with the host's IEEE arithmetic, which is the oracle. The corpus has boundary values (zeros, ones, powers of two, the smallest and largest normal and subnormal numbers, values next to those, infinities, NaNs), tie cases for every operation, cancellation, and a large pseudo-random set of bit patterns. A register-preservation program calls each helper with marker values in `SI`, `DI`, `BP`, `DS` and `ES` and with `DS` set to another segment (as `libc16/tests/arith/regs.c` does). `tests/check-abi` compiles `tests/abi/fpc.c` in each model and checks which symbols the compiler emits and how they are called.
