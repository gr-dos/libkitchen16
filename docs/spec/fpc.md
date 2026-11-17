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
- A helper uses no more than 64 bytes of stack and calls nothing outside libkitchen16's helpers.

## Arithmetic: `+ - * /`, `-`

IEEE 754 binary64:

- The result is the exact mathematical result rounded to the nearest representable `double`, ties to even. This holds for every operation, including results in the subnormal range (gradual underflow, no flush to zero) and results that overflow (an infinity of the right sign).
- Signed zeros follow IEEE 754: `+0 + -0` is `+0`; `x - x` is `+0`; `-0 + -0` is `-0`; the sign of a product or quotient is the exclusive or of the signs of the operands, also for zeros and infinities.
- Infinities: `inf + inf` is `inf`, `inf - inf` is a NaN, `inf * 0` is a NaN, `inf / inf` and `0 / 0` are NaNs, `x / 0` for finite non-zero `x` is an infinity of the right sign, `x / inf` is a zero, `inf * x` is an infinity for non-zero `x`.
- NaNs: an invalid operation (above) gives the default NaN, `0x7FF8000000000000` (positive, quiet, empty payload). When an operand is a NaN the result is that operand with its quiet bit set (`a` first if both are NaNs). A NaN's payload and sign are otherwise not significant to the tests.
- `__FDN` flips the sign bit of every value, including zeros, infinities and NaNs.
- There are no exception flags, no traps, no rounding modes and no `<fenv.h>`. A helper never raises an interrupt: not for division by zero, not for overflow, not for an invalid operation.

## Comparison: `__FDC`, `__EDC`

The result is in the flags, as if by `cmp x, y` with these operands, which make both the signed and the unsigned conditional jumps correct (the compiler uses `jge` and `jne`):

| `a` against `b` | Flags as for `cmp` of | So |
|---|---|---|
| `a < b` | `0` with `1` | `jl`, `jb` taken; `je` not |
| `a == b` | `0` with `0` | `je`, `jge`, `jle`, `jae`, `jbe` taken |
| `a > b` | `1` with `0` | `jg`, `ja` taken; `je` not |
| unordered (either is a NaN) | as `a > b` | |

`+0` and `-0` are equal. `-inf` is less than every finite value, `+inf` greater. A comparison with a NaN is therefore *greater*: `a != b` and `a > b` and `a >= b` are true, `a == b`, `a < b` and `a <= b` false. (C requires `a > b` and `a >= b` to be false for a NaN; one flags result cannot make all six comparisons right, and this choice breaks the two that a program using NaNs is least likely to test. Perl does not make NaNs.)

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

`tests/fpc` runs in every model. The programs are compiled with `-fpc`, so the compiler's own call sites exercise each helper; each reads its operands as bit patterns from a data file generated on the host (`tests/fpc/gen`), computes with C operators, and compares the result's bits with the host's IEEE arithmetic, which is the oracle. The corpus has boundary values (zeros, ones, powers of two, the smallest and largest normal and subnormal numbers, values next to those, infinities, NaNs), tie cases for every operation, cancellation, and a large pseudo-random set of bit patterns. A register-preservation program calls each helper with marker values in `SI`, `DI`, `BP`, `DS` and `ES` and with `DS` set to another segment (as `libc16/tests/arith/regs.c` does). `tests/check-abi` compiles `tests/abi/fpc.c` in each model and checks which symbols the compiler emits and how they are called.
