; fdmul.asm - __FDM (and __EDM in the compact and large models): a * b,
; IEEE 754 binary64, round to nearest even.  Docs: docs/spec/fpc.md.
;
; b is copied from SS:SI (F) or ES:SI (E) to the stack and a is pushed
; under it.  Zeros, infinities and NaNs are __fpc_special's.  Otherwise:
;   1. Unpack both to 64-bit mantissas with bit 63 set (__fpc_unpack).
;   2. The exponent is Ea + Eb - 1022 and the sign the xor of the signs.
;   3. The 128-bit product P of the mantissas (schoolbook, 4 x 4 words,
;      16 MUL instructions in two loops) is in [2^126, 2^128).  Its high
;      64 bits, with a sticky bit for the low 64 bits not being zero,
;      are the mantissa for __fpc_pack, which normalises (one bit at most),
;      rounds and packs.
; Stack: the return address, 3 saved registers, 16 bytes of operands, 24
; of locals and the call of __fpc_pack: 54 bytes in the large model.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __FDM
ifdef FARDATA
        public  __EDM
endif
        extrn   __fpc_special:CDIST
        extrn   __fpc_unpack:CDIST
        extrn   __fpc_pack:CDIST

PUSHB   macro   sg
        push    word ptr sg:[si+6]
        push    word ptr sg:[si+4]
        push    word ptr sg:[si+2]
        push    word ptr sg:[si]
        endm

        .code

__FDM   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   ss
        jmp     fdm_body
__FDM   endp

ifdef FARDATA
__EDM   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   es
        jmp     fdm_body
__EDM   endp
endif

; Locals: [bp-2] sign, [bp-4] exponent, then the product P0..P7 (words, low
; first) at [bp-20] .. [bp-6], and [bp-22] the word of a being multiplied.
fdm_body proc CDIST
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bp,sp
        mov     di,1
        call    __fpc_special
        jnc     mb_fin
        jmp     mb_done
mb_fin: lea     bx,[bp]
        call    __fpc_unpack
        push    dx                      ; sign of a
        push    ax                      ; exponent of a
        lea     bx,[bp+8]
        call    __fpc_unpack
        push    dx                      ; sign of b
        push    ax                      ; exponent of b
        pop     ax
        pop     dx
        xor     dx,[bp-2]
        add     ax,[bp-4]
        sub     ax,1022
        mov     [bp-2],dx               ; sign of the product
        mov     [bp-4],ax               ; exponent for __fpc_pack
        xor     ax,ax
        mov     cx,9
mb_z:   push    ax                      ; P0..P7 = 0, and the a word
        loop    mb_z
; P += a[i] * b, for i = 0..3; P[i+j] += a[i]*b[j] + carry
        xor     di,di                   ; DI = 2*i
mb_o:   mov     ax,[bp+di]
        mov     [bp-22],ax
        lea     bx,[bp-20]
        add     bx,di                   ; BX -> P[i]
        xor     si,si                   ; SI = 2*j
        xor     cx,cx                   ; carry
mb_i:   mov     ax,[bp-22]
        mul     word ptr [bp+si+8]
        add     ax,ss:[bx]
        adc     dx,0
        add     ax,cx
        adc     dx,0
        mov     ss:[bx],ax
        mov     cx,dx
        inc     bx
        inc     bx
        inc     si
        inc     si
        cmp     si,8
        jb      mb_i
        mov     ss:[bx],cx              ; P[i+4]
        inc     di
        inc     di
        cmp     di,8
        jb      mb_o
        mov     ax,[bp-6]               ; the high 64 bits
        mov     bx,[bp-8]
        mov     cx,[bp-10]
        mov     dx,[bp-12]
        mov     si,[bp-20]              ; sticky: the low 64 bits
        or      si,[bp-18]
        or      si,[bp-16]
        or      si,[bp-14]
        jz      mb_p
        or      dx,1
mb_p:   mov     di,[bp-4]
        mov     si,[bp-2]
        call    __fpc_pack
mb_done:
        lea     sp,[bp+16]
        pop     di
        pop     si
        pop     bp
        ret
fdm_body endp

        end
