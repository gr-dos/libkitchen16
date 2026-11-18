; fdcmp.asm - __FDC (and __EDC in the compact and large models): compare a
; (AX:BX:CX:DX) with b (8 bytes at SS:SI, or ES:SI), result in the flags as
; for CMP of 0,1 (a < b), 0,0 (a = b) or 1,0 (a > b, or unordered).  AX, BX,
; CX, DX are not changed.  Docs: docs/spec/fpc.md.
;
; b is copied to the stack.  Algorithm:
;   - a NaN (exponent 7FFh, fraction not zero) in either: "greater".
;   - both zero (of either sign): equal.
;   - different signs: the negative one is the smaller.
;   - the same sign: compare the 64 bits as an unsigned number from the top
;     word down (exponent first, then the fraction, so that is the order
;     of magnitudes); for negative values the order is reversed.
;   The flags are then set by a CMP of two registers holding 0 and 1.
; Stack: the return address, 3 saved registers, 8 bytes of b: 17 bytes
; in the large model.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __FDC
ifdef FARDATA
        public  __EDC
endif

PUSHB   macro   sg
        push    word ptr sg:[si+6]
        push    word ptr sg:[si+4]
        push    word ptr sg:[si+2]
        push    word ptr sg:[si]
        endm

        .code

__FDC   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   ss
        jmp     fdc_body
__FDC   endp

ifdef FARDATA
__EDC   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   es
        jmp     fdc_body
__EDC   endp
endif

fdc_body proc CDIST
        mov     bp,sp                   ; b at [bp]..[bp+7]
        mov     di,ax                   ; a a NaN?
        and     di,7FF0h
        cmp     di,7FF0h
        jne     c_a1
        mov     di,ax
        and     di,000Fh
        or      di,bx
        or      di,cx
        or      di,dx
        jnz     c_gt
c_a1:   mov     di,[bp+6]               ; b a NaN?
        and     di,7FF0h
        cmp     di,7FF0h
        jne     c_b1
        mov     di,[bp+6]
        and     di,000Fh
        or      di,[bp+4]
        or      di,[bp+2]
        or      di,[bp]
        jnz     c_gt
c_b1:   mov     di,ax                   ; both zero?
        and     di,7FFFh
        or      di,bx
        or      di,cx
        or      di,dx
        jnz     c_nz
        mov     di,[bp+6]
        and     di,7FFFh
        or      di,[bp+4]
        or      di,[bp+2]
        or      di,[bp]
        jz      c_eq
c_nz:   mov     si,ax
        xor     si,[bp+6]
        js      c_sd                    ; the signs differ
        cmp     ax,[bp+6]
        jne     c_diff
        cmp     bx,[bp+4]
        jne     c_diff
        cmp     cx,[bp+2]
        jne     c_diff
        cmp     dx,[bp]
        je      c_eq
c_diff: jb      c_ml
        test    ax,ax                   ; |a| > |b|: a < b if negative
        js      c_lt
        jmp     c_gt
c_ml:   test    ax,ax                   ; |a| < |b|: a < b if positive
        js      c_gt
        jmp     c_lt
c_sd:   test    ax,ax                   ; a negative: a < b
        js      c_lt
        jmp     c_gt
c_lt:   xor     si,si
        mov     di,1
        cmp     si,di                   ; 0 with 1
        jmp     c_out
c_eq:   xor     si,si
        cmp     si,si                   ; 0 with 0
        jmp     c_out
c_gt:   mov     si,1
        xor     di,di
        cmp     si,di                   ; 1 with 0
c_out:  lea     sp,[bp+8]               ; none of these change the flags
        pop     di
        pop     si
        pop     bp
        ret
fdc_body endp

        end
