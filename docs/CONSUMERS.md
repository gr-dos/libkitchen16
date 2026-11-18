# libkitchen16 Consumers

What each consumer needs from libkitchen16 (DESIGN.md §1), and how it gets it.

| Consumer | Needs | Status |
|---|---|---|
| `gr-dos/perl16` (Perl 3, M3.3 and M3.4 of #12) | the 16 `-fpc` helpers (`__FDA`, `__FDS`, `__FDM`, `__FDD`, `__FDC`, `__FDN`, `__EDA`, `__EDM`, `__EDD`, `__EDC`, `__FDI4`, `__FDU4`, `__I4FD`, `__U4FD`, `__FDFS`, `__FSFD`); `strtod`, `atof`; `atan2 cos exp log modf pow sin sqrt`; `sprintf`, `vsprintf`, `fprintf`, `printf` with `%e %f %g` (`%.20g` for every number) | M3.4 will link it. perl16's `tests/smoke/fpfmt` (`%.0f` of 2.5 gives 2, `1/3` prints `0.33333333333333331483`) is the check; it has not been run against this library yet |
| `gr-dos/tcl16` | `strtod`, `sprintf`; `sscanf` later | not started; `sscanf` is not provided |

perl16's audit (`docs/M3-RUNTIME-AUDIT.md`, sections 5.2, 8 and 9) gives the call-site counts and the formatting requirements these modules were specified from.

## Using libkitchen16

A consumer gets libkitchen16 from a release, as it gets libc16 (libc16's `docs/CONSUMERS.md`, "Using libc16"):

- **`scripts/fetch-libkitchen16`**, a copy of `consumers/fetch-libkitchen16` here, pins one release by `version` and `sha256`. It downloads `libkitchen16-<version>.zip` from the `v<version>` release, checks its SHA-256 before unpacking it, and unpacks its `libkitchen16-<version>/` directory as `build/libkitchen16`. CI and releases never follow a branch. The values in the canonical copy are placeholders until the first release; a consumer sets them to the release's (the `.sha256` asset, also in the release notes).
- **`LIBKITCHEN16_ROOT`** (default `build/libkitchen16`) is the tree the consumer's `Makefile` uses: an unpacked release, or a checkout built with `make`, whose `build/stage` has the same layout.
- A consumer that uses libkitchen16 also pins libc16 (`scripts/fetch-libc16`), a version equal to or newer than the one libkitchen16's `BUILD.txt` names, and links both libraries of its memory model.
- **The toolchain** is the Open Watcom snapshot the release was built with; `tools/fetch-watcom` in the release downloads it.

**Updating a consumer** to a new libkitchen16 is a deliberate change of its own: set `version` and `sha256` in `scripts/fetch-libkitchen16` to the new release's values in the same commit, and run its tests.

## Migration notes

- libkitchen16 is a different library from libc16, so a program that replaces Open Watcom's `printf` family keeps its calls and adds `<kitchen16.h>` after libc16's headers. Where libc16's decision 3 favours `libc16_ultoa` over `printf` for small utilities, that remains the cheaper choice for integers.
- `printf` and `fprintf` write through libc16's streams, which are unbuffered: the output of one call is handed to `fwrite` in blocks of at most 128 characters (`docs/spec/fmtio.md`), so a long line costs a few DOS calls, not one per character.
- **Comparisons with NaN, and with a literal zero.** `wcc -fpc` makes `x > y` and `x >= y` true when either is a NaN, and tests only the high 16 bits of `x` for `x == 0.0`, `x != 0.0`, `x < 0.0`, `!x`, `if (x)`, `x && y` and `x ? a : b`, so `-0.0` is non-zero and a subnormal below about `1.4e-309` is zero. A consumer that can meet these (`perl16`'s truth test of a number and its comparisons with `0.0` are the first places to look) uses the tested idioms of `docs/spec/fpc.md`, "What a program that needs IEEE results does": a zero that is not a literal, and an explicit NaN test.
- A `double` passed to `printf` is 8 bytes on the stack; a program must be compiled with `-fpc` and linked with libkitchen16 for its own `double` operations, or it links no floating-point support and fails at link time.
