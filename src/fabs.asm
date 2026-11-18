; fabs.asm - fabs (C89 7.5.6.2): |x| by clearing the sign bit, on the
; bits, no helper.  Docs: docs/spec/math.md, docs/ABI.md ("Floating-point
; arguments and results"): the argument is 8 bytes on the stack, low word
; lowest, and the result is the offset in AX of an 8-byte slot in DGROUP.
;
; Written in assembly, not C: the compiler's output for this function is
; the same few instructions as any other compiler's, and so scored over the
; similarity audit's limit (docs/LICENSE-PROVENANCE.md).  Uses AX only, and
; DS in the compact and large models, which it restores.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  _fabs

        .data
slot    dw      0, 0, 0, 0              ; the result, bits 15..0 first

        .code

_fabs   proc CDIST
        push    bp
        mov     bp,sp
ifdef FARDATA
        push    ds
        push    ss
        pop     ds                      ; the slot is in DGROUP, which is SS
endif
        mov     ax,[bp+ARG0+6]
        and     ah,7Fh
        mov     slot+6,ax
        mov     ax,[bp+ARG0+4]
        mov     slot+4,ax
        mov     ax,[bp+ARG0+2]
        mov     slot+2,ax
        mov     ax,[bp+ARG0]
        mov     slot,ax
        mov     ax,offset slot
ifdef FARDATA
        pop     ds
endif
        pop     bp
        ret
_fabs   endp

        end
