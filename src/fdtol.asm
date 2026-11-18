; fdtol.asm - __FDI4 and __FDU4: (long)a and (unsigned long)a, truncating
; toward zero, with the out of range values defined (docs/spec/fpc.md,
; "Conversions to integers").  a is AX:BX:CX:DX, the result DX:AX.
;
; With e = the unbiased exponent of a:
;   - NaN, or |a| < 1 (e < 0, subnormals and zeros): 0
;   - negative, e >= 31 (or -inf): 80000000h for both
;   - positive, signed:   e >= 31: 7FFFFFFFh
;   - positive, unsigned: e >= 32: FFFFFFFFh
;   - otherwise T = the top 32 bits of the 53-bit mantissa (bit 31 is the
;     hidden bit), shifted right by 31 - e, then negated if a is negative.
; SI = the sign, DI = 0 for signed or 1 for unsigned, BP = the exponent
; field shifted left by 4.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __FDI4
        public  __FDU4

        .code

__FDI4  proc CDIST
        push    si
        push    di
        xor     di,di
        jmp     tl_body
__FDI4  endp

__FDU4  proc CDIST
        push    si
        push    di
        mov     di,1
        jmp     tl_body
__FDU4  endp

tl_body proc CDIST
        push    bp
        mov     si,ax
        and     si,8000h
        mov     bp,ax
        and     bp,7FF0h
        cmp     bp,7FF0h
        jne     tl_fin
        mov     bp,ax                   ; infinity or NaN
        and     bp,000Fh
        or      bp,bx
        or      bp,cx
        or      bp,dx
        jnz     tl_zero                 ; NaN: 0
        mov     bp,7FF0h                ; infinity: as a huge value
        jmp     tl_ge31
tl_fin: cmp     bp,3FF0h                ; e < 0
        jb      tl_zero
        cmp     bp,41E0h                ; e < 31
        jb      tl_calc
tl_ge31:
        test    si,si
        jnz     tl_min
        test    di,di
        jz      tl_max
        cmp     bp,41E0h                ; unsigned, e = 31: the value itself
        je      tl_calc
        mov     ax,0FFFFh               ; unsigned, e >= 32
        mov     dx,ax
        jmp     tl_ret
tl_max: mov     ax,0FFFFh
        mov     dx,7FFFh
        jmp     tl_ret
tl_min: xor     ax,ax
        mov     dx,8000h
        jmp     tl_ret
tl_zero:
        xor     ax,ax
        xor     dx,dx
        jmp     tl_ret
tl_calc:
        and     ax,000Fh                ; AX:BX:CX = hidden bit and fraction
        or      ax,0010h                ; bits 52..16; >> 5 is T in BX:CX
        mov     dx,5
tl_t:   shr     ax,1
        rcr     bx,1
        rcr     cx,1
        dec     dx
        jnz     tl_t
        mov     dx,41E0h                ; shift count 31 - e
        sub     dx,bp
        rept    4
        shr     dx,1
        endm
        jz      tl_ns
tl_sh:  shr     bx,1
        rcr     cx,1
        dec     dx
        jnz     tl_sh
tl_ns:  mov     dx,bx
        mov     ax,cx
        test    si,si
        jz      tl_ret
        neg     dx
        neg     ax
        sbb     dx,0
tl_ret: pop     bp
        pop     di
        pop     si
        ret
tl_body endp

        end
