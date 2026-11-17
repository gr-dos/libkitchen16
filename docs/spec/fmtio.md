# Spec: formatted output to a stream (`fmtio`)

**Members:** one for each of `vfprintf`, `fprintf`, `vprintf` and `printf`; they share `fmt`'s engine (`docs/spec/fmt.md`)  
**Header:** `<kitchen16.h>`  
**Tests:** `tests/fmtio` (run by `tests/run-dos`)

This is the behavioural spec an implementation is written from (DESIGN.md, decision 6). It does not describe any existing implementation.

## Sources

C89 7.9.6.1 (`fprintf`), 7.9.6.3 (`printf`), 7.9.6.7 (`vfprintf`) and 7.9.6.8 (`vprintf`); libc16's `docs/spec/stdio.md` (its `FILE`, `fwrite`, `fputc`, errors) and `docs/spec/io.md`. The conversions are those of `docs/spec/fmt.md`, unchanged.

## Interface

    int __cdecl vfprintf(FILE *fp, const char *format, va_list ap);
    int __cdecl fprintf(FILE *fp, const char *format, ...);
    int __cdecl vprintf(const char *format, va_list ap);
    int __cdecl printf(const char *format, ...);

`FILE` is libc16's (`<stdio.h>`). `printf` and `vprintf` write to `stdout`. All are `__cdecl`.

## Behaviour

- The output is exactly the characters that `vsnprintf` with the same format and arguments would produce (the same conversions, flags, widths, precisions, exactness, invalid specifications), written to the stream in order, as by libc16's `fwrite` on the stream: text-mode translation, error and end-of-file flags and `errno` are libc16's.
- The return value is the number of characters written, or a negative number if an output error occurred (`fwrite` wrote fewer characters than asked). After an error the functions stop writing and return -1; `errno` is left as libc16's `fwrite` set it.
- If more than 32767 characters are written, all of them are written and the function returns -1 with `errno` set to `ERANGE`: the count does not fit in an `int`. (This differs from an output error only in `errno`.)
- A null `fp`, or a stream not open for writing, is an error, returned as above without writing; it is not a trap.
- The output is not formatted into a buffer of its own size: a call with an output of any length uses a bounded amount of memory, and the characters are handed to `fwrite` in blocks (of 128 characters or fewer) as they are produced, never one character per call. libc16's streams are unbuffered, so this is what keeps the number of DOS calls low.
- No static storage; the functions are reentrant as far as `fwrite` is. Stack use is `fmt`'s plus at most 160 bytes.

## Linking

`vfprintf` is the stream function; `fprintf`, `vprintf` and `printf` are small members that call it. A program that calls `printf` links `printf`, `vfprintf`, the engine, libc16's `fwrite` (and what it links, as libc16 decides) and `stdout`. A program that calls none of them links none of this; a program that calls `sprintf` and not `printf` links nothing of the stream code and no part of libc16's stream code on account of libkitchen16 (`tests/fmtio/members` and `tests/fmt/members` check both). The members link nothing from Open Watcom's runtime and no `fpc` helper, use 8086 instructions only, and are built for the three memory models.

## Tests

`tests/fmtio` runs in every model. A program writes the whole corpus of `tests/fmt` (the same data file) through `fprintf` to a file, and through `printf` to standard output, and the file is compared with the expected output, which `tests/fmt/gen` also writes. It also checks the return value for every line, a stream closed before the call (an error, with no output), a failing write (a full disk is not available under DOSBox-X; the test writes to a read-only handle opened with `open`, and to a stream whose handle is closed), the 32767-character limit (`%.30000d%.30000d` writes all 60,000 characters and fails with `ERANGE`), and that the output of a long format is written in blocks (a test defines its own `fwrite`, which counts its calls and keeps what it is given, and links ahead of libc16's: 1,000 characters make at most 8 calls).
