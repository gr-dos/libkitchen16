# libkitchen16 — License and Provenance

libkitchen16's own source is MIT (`LICENSE`). Licensing is a release gate: every module linked into a program needs an entry here before a release.

## Policy (DESIGN.md decision 6)

- Each module has a behavioural spec in `docs/spec/` before it is implemented. The specs cite their sources: IEEE 754, the C standard (C89 7.5 and 7.9.6, C99 7.20.1.3 where noted), perl16's runtime audit, and our own call-site experiments (`tests/abi`).
- No one working on libkitchen16 reads Open Watcom runtime source or headers or disassembles its libraries. The helper symbols and their registers were found by compiling our own C and reading the objects `wcc` produces (`docs/ABI.md`).
- Every implementation was written by a separate session that was given a copy of the repository without `src/` or its history, and told to work from the spec, the tests, the headers and the build only. The host's C library and Perl are test oracles, never sources.
- An automated audit compares each member with Open Watcom's DOS C library, software and 8087 math libraries and emulator, and reports only scores (`make audit`; "Similarity audit" below).
- AI assistance is recorded here and in the `Co-Authored-By` trailers of the commits. Material generated purely by an AI may not be protected by copyright; the MIT grant covers the human contributions, and nothing here restricts redistribution of the rest.

## Tools (not linked into programs)

| Tool | License | Notes |
|---|---|---|
| Open Watcom C `2026-10-01-Build` | Sybase Open Watcom Public License | Compiler, `wlink`, `wlib`, `wdis`. Its translator comment is in each C object but not in linked programs. |
| JWasm v2.20 with `patches/jwasm` | Sybase Open Watcom Public License | Patches from GR-DOS, MIT. |
| libc16 | MIT | A pinned dependency (`scripts/fetch-libc16`), linked with every program. |
| DOSBox-X | GPL-2.0 | Runs tests only. |
| Perl, the host C library | various | Generate and check test corpora only. |

## Modules

| Members | Source | Spec | Authors | Status |
|---|---|---|---|---|
| `fdcore`, `fdadd`, `fdmul`, `fddiv`, `fdcmp`, `fdneg`, `fdtol`, `fdfromi`, `fdfs`, `fsfd` | `src/fd*.asm`, `src/fsfd.asm` | `docs/spec/fpc.md` | Claude (Sonnet 5.5): spec and tests in the design session, code in a separate session; human review pending | Written from the spec |
| `fmt`, `vsnprintf`, `snprintf`, `vsprintf`, `sprintf` | `src/fmt.c`, `fmt.h`, `*sprintf.c`, `vsnprintf.c` | `docs/spec/fmt.md` | as above | Written from the spec |
| `vfprintf`, `fprintf`, `vprintf`, `printf` | `src/*printf.c` | `docs/spec/fmtio.md` | as above | Written from the spec |
| `strtod`, `atof` | `src/strtod.c`, `atof.c` | `docs/spec/conv.md` | as above | Written from the spec |
| the `<math.h>` functions | `src/*.c` | `docs/spec/math.md` | as above | see "math" |
| `hugeval` | `src/hugeval.c` | `docs/ABI.md` | Claude (Sonnet 5.5), design session | one constant |
| `model.inc` | `src/model.inc` | | libc16's, MIT | copied |

The tests and corpora (`tests/*/gen` and the data they write) were written by separate sessions from the specs, and generated with host Perl.

### Instructions given to each implementation session

The session was given a copy without `src/` (except `model.inc` and `hugeval.c`) or git history, and told:

- Read only the spec, `docs/ABI.md`, `docs/DESIGN.md`, `include/`, the `Makefile`, `tests/`, `scripts/` and libc16's release headers.
- Read no Open Watcom runtime source or header; disassemble no Open Watcom library or object. Using `wcc`, `wasm` and `wdis` on our own code is allowed.
- Look up no other implementation of the same functions; write from the definition (IEEE 754, the C standard, the mathematics).
- Make no edit to the spec or the tests except the `members` files, and report instead where one seems wrong.

### Notes by module

- **`fpc`:** unpack to a normalised 64-bit mantissa, shared round-to-nearest-even and pack in `fdcore`; sticky-bit alignment for add, schoolbook product for multiply, restoring division for divide.
- **`fmt`:** integers from 16-bit limbs; `%e %f %g` from the exact binary value by integer arithmetic on 16-bit limbs, so no helper is linked. The engine has one general path and no separate integral fast path; the integral cases pass the corpus.
- **`conv`:** the decimal text becomes a big integer, an exact quotient is developed by restoring division and rounded once, so the result is correctly rounded. It uses about 672 bytes of static storage, so `strtod` is not reentrant.
- **Test corrections made after an implementation session reported them** (the tests were wrong, not the code): `tests/fmtio/blocks.c` (a failing `fwrite` with zero shortfall wrote the full block) and `tests/conv/longstr.c` (four long strings lacked the exponent that cancels their zeros).

## Similarity audit

Recorded when `make audit` is run on the release candidate.
