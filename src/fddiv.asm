; fddiv.asm - __FDD (and __EDD in the compact and large models): a / b,
; IEEE 754 binary64, round to nearest even.  Docs: docs/spec/fpc.md.
;
; b is copied from SS:SI (F) or ES:SI (E) to the stack and a is pushed
; under it.  Zeros, infinities and NaNs are __fpc_special's.  Otherwise:
;   1. Unpack both to 64-bit mantissas Ma, Mb with bit 63 set.
;   2. The exponent is Ea - Eb + 1023 and the sign the xor of the signs.
;   3. Restoring long division, one quotient bit per step, 64 steps:
;      R = Ma; repeat: q = (R >= Mb); if q, R -= Mb; shift q into Q;
;      R = 2R.  (R may overflow 64 bits when doubled; SI keeps that bit,
;      and R >= Mb is then certain.)  Q = floor(Ma / Mb * 2^63), in
;      (2^62, 2^64).  Q replaces a on the stack.
;   4. The remainder, if not zero, sets the lowest bit of Q (sticky), and
;      __fpc_pack normalises (one bit at most), rounds and packs.
; Stack: the return address, 3 saved registers, 16 bytes of operands, 4 of
; locals and the call of __fpc_pack: 34 bytes in the large model.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __FDD
ifdef FARDATA
        public  __EDD
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

__FDD   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   ss
        jmp     fdd_body
__FDD   endp

ifdef FARDATA
__EDD   proc CDIST
        push    bp
        push    si
        push    di
        PUSHB   es
        jmp     fdd_body
__EDD   endp
endif

fdd_body proc CDIST
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bp,sp
        mov     di,2
        call    __fpc_special
        jnc     db_fin
        jmp     db_done
db_fin: lea     bx,[bp]
        call    __fpc_unpack
        push    dx                      ; [bp-2] sign of a
        push    ax                      ; [bp-4] exponent of a
        lea     bx,[bp+8]
        call    __fpc_unpack
        push    dx
        push    ax
        pop     ax                      ; exponent of b
        pop     dx                      ; sign of b
        xor     dx,[bp-2]
        mov     cx,[bp-4]
        sub     cx,ax
        add     cx,1023
        mov     [bp-2],dx               ; sign of the quotient
        mov     [bp-4],cx               ; exponent for __fpc_pack
        mov     ax,[bp+6]               ; R = Ma
        mov     bx,[bp+4]
        mov     cx,[bp+2]
        mov     dx,[bp]
        xor     si,si                   ; SI = bit 64 of R
        mov     [bp],si                 ; Q = 0, in a's place
        mov     [bp+2],si
        mov     [bp+4],si
        mov     [bp+6],si
        mov     di,64
db_l:   test    si,si
        jnz     db_ge
        cmp     ax,[bp+14]              ; R >= Mb ?
        ja      db_ge
        jb      db_lt
        cmp     bx,[bp+12]
        ja      db_ge
        jb      db_lt
        cmp     cx,[bp+10]
        ja      db_ge
        jb      db_lt
        cmp     dx,[bp+8]
        jb      db_lt
db_ge:  sub     dx,[bp+8]
        sbb     cx,[bp+10]
        sbb     bx,[bp+12]
        sbb     ax,[bp+14]
        xor     si,si
        stc                             ; q = 1
        jmp     db_q
db_lt:  clc                             ; q = 0
db_q:   rcl     word ptr [bp],1         ; Q = 2Q + q
        rcl     word ptr [bp+2],1
        rcl     word ptr [bp+4],1
        rcl     word ptr [bp+6],1
        shl     dx,1                    ; R = 2R
        rcl     cx,1
        rcl     bx,1
        rcl     ax,1
        sbb     si,si                   ; SI = -1 if bit 64
        dec     di
        jnz     db_l
        mov     di,ax                   ; the remainder, not zero?
        or      di,bx
        or      di,cx
        or      di,dx
        or      di,si
        mov     ax,[bp+6]
        mov     bx,[bp+4]
        mov     cx,[bp+2]
        mov     dx,[bp]
        test    di,di
        jz      db_p
        or      dx,1
db_p:   mov     di,[bp-4]
        mov     si,[bp-2]
        call    __fpc_pack
db_done:
        lea     sp,[bp+16]
        pop     di
        pop     si
        pop     bp
        ret
fdd_body endp

        end
