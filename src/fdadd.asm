; fdadd.asm - __FDA, __FDS (and __EDA, __EDS in the compact and large
; models): a + b and a - b, IEEE 754 binary64, round to nearest even.
; Docs: docs/spec/fpc.md.
;
; The right operand b is copied from SS:SI (F forms) or ES:SI (E forms) to
; the stack, a is pushed under it, so one body serves all four.  For a
; subtraction the sign of the copy of b is flipped.
;
; Algorithm (both operands finite and not zero; the rest is in
; __fpc_special):
;   1. Unpack both to 64-bit mantissas with bit 63 set and a biased
;      exponent (__fpc_unpack).
;   2. Take as "big" the operand with the larger exponent (or mantissa, if
;      the exponents are equal); the sign of the result is its sign.
;   3. Shift both mantissas right by one, so that a sum has room for a
;      carry, and the small one by its exponent difference more; the bits
;      shifted out are ORed into the lowest bit (a sticky bit).  The 11
;      low bits an unpacked mantissa has are zero, so at most a one-bit
;      cancellation can need more than the bits kept, and then nothing
;      was shifted out of the small one.
;   4. Add or subtract (by the signs) the mantissas.  A zero difference is
;      +0.
;   5. __fpc_pack normalises, rounds and packs.
; Stack: the return address, SI, DI, BP, 16 bytes of operands, 8 of locals,
; and the call of __fpc_pack: 48 bytes in the large model.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __FDA
        public  __FDS
ifdef FARDATA
        public  __EDA
        public  __EDS
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

__FDA   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   ss
        jmp     fda_add
__FDA   endp

__FDS   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   ss
        jmp     fda_sub
__FDS   endp

ifdef FARDATA
__EDA   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   es
        jmp     fda_add
__EDA   endp

__EDS   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   es
        jmp     fda_sub
__EDS   endp
endif

fda_sub proc CDIST
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bp,sp
        xor     word ptr [bp+14],8000h  ; a - b = a + (-b)
        jmp     fda_body
fda_sub endp

fda_add proc CDIST
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bp,sp
        jmp     fda_body
fda_add endp

fda_body proc CDIST
        xor     di,di
        call    __fpc_special
        jnc     ab_fin
        jmp     ab_done
ab_fin: lea     bx,[bp]
        call    __fpc_unpack
        push    dx                      ; [bp-2] sign of a
        push    ax                      ; [bp-4] exponent of a
        lea     bx,[bp+8]
        call    __fpc_unpack
        push    dx                      ; [bp-6] sign of b
        push    ax                      ; [bp-8] exponent of b
; the bigger operand: SI -> its mantissa, DI -> the other's
        mov     ax,[bp-4]
        mov     dx,[bp-8]
        cmp     ax,dx
        jg      ab_abig
        jl      ab_bbig
        mov     si,6                    ; equal exponents: compare mantissas
ab_cmp: mov     cx,[bp+si]
        cmp     cx,[bp+si+8]
        ja      ab_abig
        jb      ab_bbig
        sub     si,2
        jns     ab_cmp
ab_abig:
        lea     si,[bp]
        lea     di,[bp+8]
        jmp     ab_go
ab_bbig:
        lea     si,[bp+8]
        lea     di,[bp]
        mov     ax,[bp-2]               ; swap the signs and exponents, so
        xchg    ax,[bp-6]               ; that [bp-2], [bp-4] are the big
        mov     [bp-2],ax               ; operand's, [bp-6], [bp-8] the small
        mov     ax,[bp-4]
        xchg    ax,[bp-8]
        mov     [bp-4],ax
ab_go:  mov     cx,[bp-4]
        sub     cx,[bp-8]
        inc     cx                      ; CX = shift of the small one (>= 1)
        shr     word ptr ss:[si+6],1    ; big >> 1
        rcr     word ptr ss:[si+4],1
        rcr     word ptr ss:[si+2],1
        rcr     word ptr ss:[si],1
        cmp     cx,64
        jb      ab_sl
        mov     word ptr ss:[di],1      ; shifted out entirely: only sticky
        mov     word ptr ss:[di+2],0
        mov     word ptr ss:[di+4],0
        mov     word ptr ss:[di+6],0
        jmp     ab_al
ab_sl:  shr     word ptr ss:[di+6],1
        rcr     word ptr ss:[di+4],1
        rcr     word ptr ss:[di+2],1
        rcr     word ptr ss:[di],1
        jnc     ab_nj
        or      word ptr ss:[di],1      ; sticky
ab_nj:  loop    ab_sl
ab_al:  mov     ax,ss:[si+6]
        mov     bx,ss:[si+4]
        mov     cx,ss:[si+2]
        mov     dx,ss:[si]
        mov     si,[bp-2]
        cmp     si,[bp-6]
        jne     ab_sub
        add     dx,ss:[di]              ; same signs: add
        adc     cx,ss:[di+2]
        adc     bx,ss:[di+4]
        adc     ax,ss:[di+6]
        jmp     ab_pack
ab_sub: sub     dx,ss:[di]              ; opposite signs: big - small >= 0
        sbb     cx,ss:[di+2]
        sbb     bx,ss:[di+4]
        sbb     ax,ss:[di+6]
        mov     si,ax
        or      si,bx
        or      si,cx
        or      si,dx
        jnz     ab_pack
        jmp     ab_done                 ; exactly zero: +0 (AX..DX are 0)
ab_pack:
        mov     di,[bp-4]
        inc     di                      ; the mantissa's bit 62 is the top
        mov     si,[bp-2]
        call    __fpc_pack
ab_done:
        lea     sp,[bp+16]
        pop     di
        pop     si
        pop     bp
        ret
fda_body endp

        end
