; fdcore.asm - code shared by the arithmetic helpers __FDA __FDS __FDM __FDD
; (docs/spec/fpc.md).  Not helpers themselves: internal routines called
; with CALL, by the members fdadd, fdmul and fddiv.  They may change any
; register except BP (and DS, ES, SS); every operand is on the stack, so
; every memory access through BX is made through SS.
;
; An operand is 8 bytes, low word first.  A frame has operand a at [bp] and
; operand b at [bp+8].
;
;   __fpc_classify  BX -> operand.  Returns AX = 0 finite non-zero,
;                   1 zero, 2 infinity, 3 NaN.  Changes AX, DX.
;
;   __fpc_unpack    BX -> a finite non-zero operand.  Replaces it with its
;                   64-bit mantissa, normalised so that bit 63 is set
;                   (value = M / 2^63 * 2^(E-1023)).  Returns AX = E (the
;                   biased exponent; below 1 for a subnormal, which is
;                   normalised), DX = sign (0 or 8000h).  Changes AX CX DX.
;
;   __fpc_special   Frame in BP, DI = 0 add, 1 multiply, 2 divide.  If an
;                   operand is a NaN, an infinity or a zero, the result is
;                   known: CF set, result in AX:BX:CX:DX.  Otherwise CF
;                   clear (both operands finite and non-zero).
;
;   __fpc_pack      AX:BX:CX:DX = M (not zero), DI = E, SI = sign.  The
;                   value is M / 2^63 * 2^(E-1023) (M need not be
;                   normalised; bits below the rounding point must hold a
;                   sticky bit).  Normalises, rounds to nearest even at bit
;                   11 (gradual underflow below E = 1), and returns the
;                   binary64 in AX:BX:CX:DX.  Changes SI, DI.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __fpc_classify
        public  __fpc_unpack
        public  __fpc_special
        public  __fpc_pack

        .code

; ---------------------------------------------------------------------
; Class of the operand at SS:BX.
__fpc_classify proc CDIST
        mov     dx,ss:[bx+6]
        mov     ax,dx
        and     ax,7FF0h                ; exponent field, in place
        and     dx,000Fh
        or      dx,ss:[bx+4]
        or      dx,ss:[bx+2]
        or      dx,ss:[bx]              ; DX = fraction, 0 or not
        cmp     ax,7FF0h
        je      cl_spec
        or      ax,ax
        jnz     cl_fin
        or      dx,dx
        jnz     cl_fin                  ; subnormal
        mov     ax,1                    ; zero
        ret
cl_fin: xor     ax,ax
        ret
cl_spec:
        mov     ax,2                    ; infinity
        or      dx,dx
        jz      cl_ret
        mov     ax,3                    ; NaN
cl_ret: ret
__fpc_classify endp

; ---------------------------------------------------------------------
; Unpack the finite non-zero operand at SS:BX in place.
__fpc_unpack proc CDIST
        mov     dx,ss:[bx+6]
        and     dx,8000h                ; DX = sign
        mov     ax,ss:[bx+6]
        and     ax,7FF0h
        mov     cl,4
        shr     ax,cl                   ; AX = exponent field
        and     word ptr ss:[bx+6],000Fh        ; fraction bits 51..48
        or      ax,ax
        jnz     up_norm
        mov     ax,1                    ; subnormal: exponent 1, no hidden bit
        jmp     up_move
up_norm:
        or      word ptr ss:[bx+6],0010h        ; the hidden bit, bit 52
up_move:
        mov     cx,11                   ; bit 52 to bit 63
up_l1:  shl     word ptr ss:[bx],1
        rcl     word ptr ss:[bx+2],1
        rcl     word ptr ss:[bx+4],1
        rcl     word ptr ss:[bx+6],1
        loop    up_l1
up_l2:  test    word ptr ss:[bx+6],8000h        ; a subnormal: shift until
        jnz     up_done                         ; bit 63 is set
        shl     word ptr ss:[bx],1
        rcl     word ptr ss:[bx+2],1
        rcl     word ptr ss:[bx+4],1
        rcl     word ptr ss:[bx+6],1
        dec     ax
        jmp     up_l2
up_done:
        ret
__fpc_unpack endp

; ---------------------------------------------------------------------
; Results that do not need arithmetic.  SI = class of a, CX = class of b.
__fpc_special proc CDIST
        lea     bx,[bp]
        call    __fpc_classify
        mov     si,ax
        lea     bx,[bp+8]
        call    __fpc_classify
        mov     cx,ax
        cmp     si,3
        jne     sp_a_ok
        mov     ax,[bp+6]               ; a is a NaN: a, quiet
        or      ax,0008h
        jmp     sp_load_a
sp_a_ok:
        cmp     cx,3
        jne     sp_b_ok
        mov     ax,[bp+14]              ; b is a NaN: b, quiet
        or      ax,0008h
        mov     bx,[bp+12]
        mov     cx,[bp+10]
        mov     dx,[bp+8]
        jmp     sp_yes
sp_b_ok:
        mov     ax,si
        or      ax,cx
        jnz     sp_some
        clc                             ; both finite, non-zero
        ret
sp_some:
        or      di,di
        je      sp_add
        cmp     di,1
        je      sp_mul
; divide: inf/inf and 0/0 are invalid; inf/x and x/0 are infinite,
; x/inf and 0/x are zero
        cmp     si,cx
        je      sp_nan
        cmp     si,2
        je      sp_inf
        cmp     cx,1
        je      sp_inf
        jmp     sp_zero
; multiply: inf*0 is invalid; otherwise an infinity, or a zero
sp_mul:
        cmp     si,2
        jne     sp_m1
        cmp     cx,1
        je      sp_nan
sp_m1:  cmp     si,1
        jne     sp_m2
        cmp     cx,2
        je      sp_nan
sp_m2:  cmp     si,2
        je      sp_inf
        cmp     cx,2
        je      sp_inf
        jmp     sp_zero
; add: inf+inf of opposite signs is invalid; an infinity wins; 0+0 is a
; zero of the common sign, or +0; 0+x is x
sp_add: cmp     si,2
        jne     sp_a1
        cmp     cx,2
        jne     sp_ret_a
        mov     ax,[bp+6]
        xor     ax,[bp+14]
        js      sp_nan
        jmp     sp_ret_a
sp_a1:  cmp     cx,2
        je      sp_ret_b
        cmp     si,cx
        jne     sp_a2
        mov     ax,[bp+6]               ; two zeros
        xor     ax,[bp+14]
        js      sp_zero_pos
        jmp     sp_ret_a
sp_a2:  cmp     si,1
        je      sp_ret_b
sp_ret_a:
        mov     ax,[bp+6]
sp_load_a:
        mov     bx,[bp+4]
        mov     cx,[bp+2]
        mov     dx,[bp]
        jmp     sp_yes
sp_ret_b:
        mov     ax,[bp+14]
        mov     bx,[bp+12]
        mov     cx,[bp+10]
        mov     dx,[bp+8]
        jmp     sp_yes
sp_inf: mov     ax,[bp+6]               ; infinity, sign a xor b
        xor     ax,[bp+14]
        and     ax,8000h
        or      ax,7FF0h
        jmp     sp_zero_rest
sp_zero:
        mov     ax,[bp+6]               ; zero, sign a xor b
        xor     ax,[bp+14]
        and     ax,8000h
        jmp     sp_zero_rest
sp_nan: mov     ax,7FF8h                ; the default NaN
        jmp     sp_zero_rest
sp_zero_pos:
        xor     ax,ax
sp_zero_rest:
        xor     bx,bx
        xor     cx,cx
        xor     dx,dx
sp_yes: stc
        ret
__fpc_special endp

; ---------------------------------------------------------------------
; Normalise, round and pack.  BP is used as a scratch register and kept.
__fpc_pack proc CDIST
        push    bp
; normalise: shift left until bit 63 is set, E down by the shift
np_norm:
        or      ax,ax
        js      np_normd
        jnz     np_bit
        mov     ax,bx                   ; top word zero: shift by 16
        mov     bx,cx
        mov     cx,dx
        xor     dx,dx
        sub     di,16
        jmp     np_norm
np_bit: shl     dx,1
        rcl     cx,1
        rcl     bx,1
        rcl     ax,1
        dec     di
        jmp     np_norm
np_normd:
        cmp     di,2047
        jge     np_inf
        cmp     di,1
        jge     np_round
; E < 1: shift right by 1-E, keeping the bits lost in the lowest bit
        mov     bp,1
        sub     bp,di
        mov     di,1
        cmp     bp,64
        jb      np_w
        xor     ax,ax                   ; everything is lost
        xor     bx,bx
        xor     cx,cx
        mov     dx,1
        jmp     np_round
np_w:   cmp     bp,16
        jb      np_dl
        neg     dx                      ; CF = DX not zero
        mov     dx,cx
        mov     cx,bx
        mov     bx,ax
        mov     ax,0
        jnc     np_w1
        or      dx,1
np_w1:  sub     bp,16
        jmp     np_w
np_dl:  or      bp,bp
        jz      np_round
np_dl1: shr     ax,1
        rcr     bx,1
        rcr     cx,1
        rcr     dx,1
        jnc     np_dl2
        or      dx,1
np_dl2: dec     bp
        jnz     np_dl1
; round to nearest even at bit 11: add 3FFh, or 400h if bit 11 is set
np_round:
        mov     bp,03FFh
        test    dx,0800h
        jz      np_r1
        inc     bp
np_r1:  add     dx,bp
        adc     cx,0
        adc     bx,0
        adc     ax,0
        jnc     np_r2
        rcr     ax,1                    ; M was 2^64 - ...: now 2^63
        rcr     bx,1
        rcr     cx,1
        rcr     dx,1
        inc     di
np_r2:  cmp     di,2047
        jge     np_inf
; pack: BP:AX:BX:CX = M >> 11 (bit 52 is the hidden bit), plus
; (E-1) << 52, so that the hidden bit adds the last 1 of E, and a
; subnormal (E = 1, no hidden bit) gets an exponent field of 0
        xor     bp,bp
        rept    5
        shl     dx,1
        rcl     cx,1
        rcl     bx,1
        rcl     ax,1
        rcl     bp,1
        endm
        dec     di
        rept    4
        shl     di,1
        endm
        add     bp,di
        or      bp,si
        mov     dx,cx
        mov     cx,bx
        mov     bx,ax
        mov     ax,bp
        jmp     np_exit
np_inf: mov     ax,si
        or      ax,7FF0h
        xor     bx,bx
        xor     cx,cx
        xor     dx,dx
np_exit:
        pop     bp
        ret
__fpc_pack endp

        end
