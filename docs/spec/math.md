# Spec: `<math.h>`

**Members:** one per function (a function that another needs may share a helper member, which is then linked with it, and nothing else); `HUGE_VAL`'s object, `__kitchen16_huge_val`, is its own member  
**Header:** `<math.h>` (libkitchen16's own)  
**Tests:** `tests/math` (run by `tests/run-dos`)

This is the behavioural spec an implementation is written from (DESIGN.md, decision 6). It does not describe any existing implementation.

## Sources

C89 7.5 (the mathematical functions, their domain and range errors); IEEE 754-1985; the mathematics of each function: Taylor and minimax (Remez) polynomial and rational approximations, argument reduction by `ln 2` and `pi/2` in several parts, the identities between the functions, and the digits of `pi` and of `ln 2` (public mathematical constants); textbook references such as Cody and Waite's *Software Manual for the Elementary Functions* and Muller's *Elementary Functions*. The host's `libm` is the test oracle only. No other runtime's source is read, and none of its libraries is disassembled.

## Interface

Every function is `__cdecl`, takes and returns `double` (`docs/ABI.md`, "Floating-point arguments and results"), and is declared in `<math.h>`:

    double __cdecl acos(double x);    double __cdecl asin(double x);
    double __cdecl atan(double x);    double __cdecl atan2(double y, double x);
    double __cdecl ceil(double x);    double __cdecl cos(double x);
    double __cdecl cosh(double x);    double __cdecl exp(double x);
    double __cdecl fabs(double x);    double __cdecl floor(double x);
    double __cdecl fmod(double x, double y);
    double __cdecl frexp(double x, int *exp);
    double __cdecl ldexp(double x, int exp);
    double __cdecl log(double x);     double __cdecl log10(double x);
    double __cdecl modf(double x, double *iptr);
    double __cdecl pow(double x, double y);
    double __cdecl sin(double x);     double __cdecl sinh(double x);
    double __cdecl sqrt(double x);    double __cdecl tan(double x);
    double __cdecl tanh(double x);

and the macro `HUGE_VAL`, a positive `double` expression equal to `+inf` (`(__kitchen16_huge_val)`, an object of type `const double`; it is not usable in an initialiser of a static object). `<math.h>` includes libc16's `<_libc16.h>`, so any other memory model is a compile error, and declares nothing else.

The functions are compiled with `-fpc`, so they use the helpers of `docs/spec/fpc.md` for their arithmetic: that is why `fpc` is independent of everything else, and why a program that links a math function links the helpers it needs.

## Results

A result is **exact** or **within *n* ulps**, the difference from the exact mathematical value expressed in units in the last place of the exact value's binade (a result of 0 where the exact value is non-zero is a failure of the bound unless it is the documented underflow). The bounds:

| Functions | Result |
|---|---|
| `fabs`, `floor`, `ceil`, `frexp`, `modf`, `ldexp`, `fmod` | exact (`ldexp`'s result in the subnormal range is rounded to nearest even) |
| `sqrt` | correctly rounded (the nearest `double`, ties to even): the same as IEEE 754's square root |
| `exp`, `log`, `log10`, `sin`, `cos`, `tan`, `atan`, `atan2`, `asin`, `acos`, `sinh`, `cosh`, `tanh` | within 4 ulps, for every finite argument |
| `pow` | within 4 ulps, and **exact whenever the exact result is a `double`**: `pow(2, n)` for every integer `n` whose result is finite and not below `2^-1074`, and `pow(x, n)` for integers `x`, `n >= 0` with `x^n` exactly representable (`10^22` is the largest power of ten), `pow(x, 1)` is `x`, `pow(x, 0)` is `1` for every `x` (also NaN) |
| `exp(0)`, `log(1)`, `sin(0)`, `tan(0)`, `atan(0)`, `asin(0)`, `sinh(0)`, `tanh(0)`, `cos(0)`, `cosh(0)`, `acos(1)` | exactly `1` or `0` (with the sign of the argument for the odd functions) |
| `sin`, `cos`, `tan` | the bound holds for every finite `x`, however large: the argument is reduced against `pi` carried to 1,200 bits (Payne and Hanek's method, or any exact one), so that `sin(1e22)` is right |
| `log10` | `log10` of the `double` nearest `10^n` is exactly `n`, for every integer `n` from -307 to 308 |

The functions are not required to be monotonic, nor to satisfy identities to the last bit.

## Special values and errors

A function does not trap and does not set `errno` except where it says so; it never clears `errno`. A **domain error** sets `errno` to `EDOM` and returns the default quiet NaN, `0x7FF8000000000000`. A **range error** sets `errno` to `ERANGE`: for an overflow it returns `HUGE_VAL` with the sign of the exact result, and for an underflow it returns a zero (with that sign) and also sets `ERANGE`; a result that is subnormal and non-zero is not an underflow. A NaN argument gives a NaN result (the argument, quieted) with no error, except `pow(x, 0)`, which is `1`, and `pow(1, y)`, which is `1`, for any `y`.

| Function | Domain error | Range error | Infinities and other special arguments |
|---|---|---|---|
| `acos`, `asin` | `|x| > 1` | | `acos(1)` is 0, `asin(±1)` is `±pi/2` |
| `atan` | | | `atan(±inf)` is `±pi/2` |
| `atan2(y, x)` | | | the IEEE values for zeros and infinities, no error: `atan2(±0, +0)` is `±0`, `atan2(±0, -0)` is `±pi`, `atan2(±0, x<0)` is `±pi`, `atan2(±0, x>0)` is `±0`, `atan2(y, ±0)` is `±pi/2` for `y` of that sign, `atan2(±inf, +inf)` is `±pi/4`, `atan2(±inf, -inf)` is `±3pi/4` |
| `ceil`, `floor`, `fabs` | | | the argument for `±inf`, NaN and every integral value; `ceil(-0.5)` is `-0` |
| `cos`, `sin`, `tan` | `x` is `±inf` | | `sin(-0)` is `-0` |
| `cosh`, `sinh` | | overflow | `cosh(±inf)` is `+inf`, `sinh(±inf)` is `±inf`, both without error |
| `tanh` | | | `tanh(±inf)` is `±1`; no error |
| `exp` | | overflow, underflow | `exp(+inf)` is `+inf` and `exp(-inf)` is `+0`, without error |
| `fmod(x, y)` | `y` is `0`, or `x` is `±inf`, when neither argument is a NaN (a NaN argument gives a NaN, no error) | | the result has the sign of `x` and a magnitude less than `|y|`; `fmod(x, ±inf)` is `x`; `fmod(±0, y)` is `±0` |
| `frexp(x, &e)` | | | for finite non-zero `x`: `0.5 <= |m| < 1` and `x = m * 2^e`, also for subnormals; for `0`, `±inf` and NaN: returns `x` and stores 0 |
| `ldexp(x, e)` | | overflow, underflow | exact in the range; `x` itself for `0`, `±inf`, NaN. `e` is an `int` of any value |
| `log`, `log10` | `x < 0` | `x = 0`: returns `-HUGE_VAL` with `ERANGE` | `log(+inf)` is `+inf`, no error; `log(-0)` as `log(+0)` |
| `modf(x, &i)` | | | `|result|` is the fractional part and `*i` the integral part, both with the sign of `x`; for `±inf`: returns `±0`, `*i` is `x`; for NaN: both NaN |
| `pow(x, y)` | finite `x < 0` and finite non-integer `y` | overflow, underflow; `x = 0` and finite `y < 0`: returns `HUGE_VAL` (negative if `x` is `-0` and `y` an odd integer) with `ERANGE` | `pow(0, 0)` is `1`; `pow(x, ±inf)`, `pow(±inf, y)` and `pow(-1, ±inf)` follow IEEE 754 / C99 Annex F, without error; `pow(x<0, integer y)` has the sign `-` when `y` is odd |
| `sqrt` | `x < 0` (`sqrt(-0)` is `-0`, not an error) | | `sqrt(+inf)` is `+inf` |

## Size and linking

- The members use no static storage except constant tables, no allocation, and at most 448 bytes of stack, counting the call's arguments and return address and the helpers' own use (`pow` and `sin` and the others included). `tests/stack` measures it on the final library in all three models, with the real helpers, on the values that reach every path (error branches, special values, the widest argument reduction): the worst case is `pow`, 396 bytes in the large model (380 small, 384 compact), and the bound leaves about 50 bytes for code generation to change; `docs/DESIGN.md` §8 has the table. A consumer that links a math function must allow that on its stack. A function returning a `double` returns it as `docs/ABI.md` says: through an 8-byte static slot (its own), so it is not reentrant from an interrupt handler.
- A program that calls one function links that function, the helpers it needs and the members it shares; `tests/math/members` lists them. `fabs`, `floor`, `ceil`, `frexp`, `modf` and `ldexp` operate on the bits (they link no helper, or at most a comparison); `sqrt` may be a bit-by-bit exact integer square root, or use the helpers.
- No member links anything from Open Watcom's runtime or from libc16 except `errno`. Use 8086 instructions only. Built for the three memory models.

## Tests

`tests/math` runs in every model. Each line of the data file gives the function, its argument bits (and the second argument), and the expected result's bits computed on the host with Perl's `POSIX` functions over the host's `libm` (`tests/math/gen`); a program compares each with the bound of the table (ulps are computed on the integer bit patterns, so that a result in a different binade is an error of a few ulps only if it is). The corpus has: the special values of every row above, every exact result listed, a sweep of each function over its domain at many scales (including 1e-300, 1e300 and the subnormal range), arguments next to every breakpoint of the method (`ln 2` multiples, `pi/2` multiples, 1, 0.5, 709.78 and 710 for `exp`, 1e22 and 2^53 for `sin`), integer powers, and a large pseudo-random set. Each case also checks `errno`, which the program clears before it calls. A separate program checks each function's selective linking and that no helper is linked by those that need none.
