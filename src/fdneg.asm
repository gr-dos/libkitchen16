; fdneg.asm - __FDN: -a.  Flips the sign bit of any value (zeros,
; infinities and NaNs too).  Docs: docs/spec/fpc.md.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __FDN

        .code

__FDN   proc CDIST
        xor     ax,8000h
        ret
__FDN   endp

        end
