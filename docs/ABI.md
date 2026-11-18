# libkitchen16 ABI

**Toolchain:** open-watcom-v2 `2026-10-01-Build` (the same pin as libc16)  
**Target:** DOS real-mode `.EXE`, 8086 instructions only; the small, compact and large memory models

This is the contract between libkitchen16 and the programs that link it. It adds to libc16's (`docs/ABI.md` there): the memory models, the `__cdecl` convention, `DS` and `SS`, callbacks and startup are libc16's, unchanged. Statements marked *tested* are checked by `make test` (`tests/check-abi`, `tests/run-dos`).

## Consumer build

Compile as a libc16 consumer does, with software floating point and libkitchen16's headers before libc16's; link libkitchen16's library of the model, then libc16's. `LIBKITCHEN16` and `LIBC16` are trees in the release layout: an unpacked release, or a checkout's `build/stage`:

```text
wcc -zl -zld -x -ms -0 -s -fpc -bt=dos -I$(LIBKITCHEN16)/include -I$(LIBC16)/include ...
wlink option nodefaultlibs format dos ... library $(LIBKITCHEN16)/lib/LIBK16.LIB library $(LIBC16)/lib/LIBC16.LIB
```

| Model | `wcc` | Libraries |
|---|---|---|
| small | `-ms` | `lib/LIBK16.LIB`, libc16's `LIBC16.LIB` |
| compact | `-mc` | `lib/LIBK16C.LIB`, `LIBC16C.LIB` |
| large | `-ml` | `lib/LIBK16L.LIB`, `LIBC16L.LIB` |

| Flag | Why |
|---|---|
| `-fpc` | floating-point operations become calls to the helpers below. Without it `wcc` emits x87 instructions and emulator fixups that need an 8087 or an emulator, neither of which a libkitchen16 program has |
| the libc16 flags | as libc16's `docs/ABI.md`, "Consumer build" |

Include libc16's headers first where a program uses both: `<kitchen16.h>` needs libc16's `size_t`, `va_list` and `FILE`. Any model other than the three is a compile error (`<math.h>` and `<kitchen16.h>` include libc16's `<_libc16.h>`). The libkitchen16 library of one model works only with libc16's of the same model.

## Floating-point arguments and results

For a `__cdecl` function (every public libkitchen16 function), from our own call-site experiments (`tests/abi/dblcdecl.c`, `wdis -a` of the objects `wcc -fpc` produces):

- **A `double` argument** is pushed as four words, the low word last: it takes 8 bytes of the stack with bits 15 to 0 at the lowest address. Arguments are pushed right to left and the caller pops them, as for any `__cdecl` call (*tested*). In a variadic call (`sprintf(buf, "%g", x)`) a `double` is the same 8 bytes, so libc16's `va_arg(ap, double)` reads it; a `float` is promoted first.
- **A `double` result** is returned as the offset in `AX` of an 8-byte slot in `DGROUP`, bits 15 to 0 first; the caller copies it at once (`mov bx,ax`, then `ss:6[bx]` ...). The callee keeps the slot in its own static data, one per `return`, which `wcc` allocates when it compiles a `double`-returning function in C (*tested*). Such a function is not reentrant from an interrupt handler.
- **A `double` default-convention argument or result** (a program's own functions) is `AX:BX:CX:DX` in registers, `AX` holding bits 63 to 48; a library function does not take or return one.

## Floating-point helpers

What `wcc -fpc` calls instead of x87 instructions, and what `libkitchen16` provides (`docs/spec/fpc.md`). The symbols, the registers and the order of the operands were found by compiling one-line C functions with the consumer flags and reading `EXTRN` and the call sites in our own objects (`tests/abi/fpc.c`, checked by `tests/check-abi`: *tested*). They are private to libkitchen16 and the compiler.

| Symbol | Operation | In | Out |
|---|---|---|---|
| `__FDA`, `__FDS`, `__FDM`, `__FDD` | `a + b`, `a - b`, `a * b`, `a / b` | `a` = `AX:BX:CX:DX`, `b` = 8 bytes at `SS:SI` | `AX:BX:CX:DX` |
| `__EDA`, `__EDS`, `__EDM`, `__EDD` | the same, `b` reached through a far pointer | `b` at `ES:SI` | `AX:BX:CX:DX` |
| `__FDC`, `__EDC` | compare `a` with `b` | as above | the flags, as `cmp` of two integers; the compiler then branches on `je jne jl jge jle jg` only (spec, "Comparison") |
| `__FDN` | `-a` | `AX:BX:CX:DX` | `AX:BX:CX:DX` |
| `__FDI4`, `__FDU4` | `(long)a`, `(unsigned long)a`; `(int)`, `(unsigned)` read `AX` | `AX:BX:CX:DX` | `DX:AX` |
| `__I4FD`, `__U4FD` | `(double)` of `long`, `unsigned long`; an `int` is widened first | `DX:AX` | `AX:BX:CX:DX` |
| `__FDFS`, `__FSFD` | `(float)a`, `(double)f` | `AX:BX:CX:DX`, `DX:AX` | `DX:AX`, `AX:BX:CX:DX` |

- The small model uses the `F` forms only; the compact and large models use the `E` forms when the right operand is reached through a far pointer (*tested*).
- Calls are near in the small and compact models and far in the large model (*tested*).
- The 8-byte operand is at `SS:SI`, not `DS:SI`: `DS` may hold any segment in the compact and large models. It is never written.
- Every helper changes `AX`, `BX`, `CX`, `DX` and the flags only (`__FDC` and `__EDC` only the flags), and preserves `SI`, `DI`, `BP`, `DS`, `ES`, `SS` and the direction flag. The call-site experiments did not establish which other registers callers keep live across a helper call, so the spec preserves all of them, as libc16 does for registers its experiments leave open.
- Not provided, so a program using them fails to link rather than misbehave: the helpers for `long long` conversions (`__FDI8`, `__I8FD`) and for `float` arithmetic (`__FSA` and the other `__FS*`).

## Comparisons

From our own call-site experiments (`tests/abi/fpccmp.c`, `tests/check-abi`; `tests/fpc/nancmp.c` runs them): each of `== != < <= > >=` is one `__FDC` (or `__EDC`) call followed by a branch on a signed condition (`je jne jl jge jle jg`), with the operands in the order written. `<` and `>=` test the same flags and are complements, as are `<=` and `>`, so a helper cannot make all six operators right for a NaN: an unordered pair is reported as *greater*, so `>` and `>=` are true for a NaN and the other four are right (`docs/spec/fpc.md`, "What the compiler does with the flags" shows the cases). A comparison with a literal `0.0`, `!x`, `if (x)`, `x && y` and `x ? a : b` call no helper: the compiler tests the high 16 bits of the operand, which is wrong for `-0.0` and for subnormals below about `1.4e-309`, and a NaN is non-zero. Both are the compiler's convention, not the helpers'; the spec gives the idioms for a program that needs IEEE results.

## The public functions

Every function declared in `<kitchen16.h>` and `<math.h>` is `__cdecl`, symbol `_name`, with libc16's rules for registers, far pointers and the direction flag. `errno` is libc16's. `HUGE_VAL` is the object `__kitchen16_huge_val`.

## Known issues

- `x > y` and `x >= y` are true when either is a NaN, and a comparison with a literal `0.0`, `!x` and `if (x)` test only the high 16 bits (`-0.0` is non-zero and negative, a tiny subnormal is zero). Both come from `wcc -fpc`'s call sites and cannot be fixed in the helpers (`docs/spec/fpc.md`, "Comparison"; the workarounds are there and tested).

- `wcc` folds an infinity constant in a `return` to zero (`double f(void) { return 1e999; }` returns 0). An infinity constant in a static initialiser is correct. That is why `HUGE_VAL` is an object.
- A `double`-returning `__cdecl` function is not reentrant (its result is in a static slot).
- wcc writes the absolute path of the source file into each object's module name, so the libraries depend on the checkout directory, as libc16's do.
