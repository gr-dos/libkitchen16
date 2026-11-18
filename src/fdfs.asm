; fdfs.asm - __FDFS: (float)a.  AX:BX:CX:DX in, DX:AX out.  Rounds to
; nearest even, with gradual underflow and overflow to infinity.  Docs:
; docs/spec/fpc.md.
;
;   1. T = the top 32 bits of the 53-bit mantissa (hidden bit = bit 31),
;      and BP = a sticky word, not zero if any lower bit is.
;   2. exponent field 7FFh: infinity, or a NaN, quiet, with the top bits of
;      its fraction.  Exponent field 0 (zero, or a double subnormal, far
;      below the smallest float): zero with the sign.
;   3. e8 = E - 896.  e8 >= 255: infinity.  e8 < 1: T is shifted right by
;      1 - e8 (lost bits into the sticky word) and e8 becomes 1.
;   4. The float's mantissa is T >> 8 (the low byte R of T and the sticky
;      word decide the rounding: round up if R > 80h, or R = 80h and the
;      sticky word is not zero or the mantissa is odd).
;   5. result = sign + ((e8 - 1) << 23) + mantissa: a hidden bit that is
;      set, or a carry from the rounding, adds to the exponent.  An
;      exponent that rounds up to 255 is an infinity (its mantissa is 0).
; SI = the sign, DI = E, BX = the low word of T for the rounding.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __FDFS

        .code

__FDFS  proc CDIST
        push    si
        push    di
        push    bp
        mov     si,ax
        mov     di,ax
        and     si,8000h
        and     di,7FF0h
        mov     bp,cx                   ; sticky: fraction bits 20..0 ...
        and     bp,001Fh
        or      bp,dx                   ; ... below T
        and     ax,000Fh
        or      ax,0010h
        mov     dx,5                    ; T = (AX:BX:CX) >> 5, in BX:CX
fd_t:   shr     ax,1
        rcr     bx,1
        rcr     cx,1
        dec     dx
        jnz     fd_t
        mov     dx,bx
        mov     ax,cx                   ; DX:AX = T
        cmp     di,7FF0h
        je      fd_spec
        or      di,di
        jz      fd_zero
        mov     cl,4
        shr     di,cl
        sub     di,896                  ; e8
        cmp     di,255
        jge     fd_inf
        cmp     di,1
        jge     fd_round
        mov     cx,1                    ; subnormal float: shift T right
        sub     cx,di
        mov     di,1
        cmp     cx,32
        jb      fd_dl
        xor     dx,dx                   ; everything is lost
        xor     ax,ax
        mov     bp,1
        jmp     fd_round
fd_dl:  shr     dx,1
        rcr     ax,1
        jnc     fd_dn
        or      bp,1
fd_dn:  loop    fd_dl
fd_round:
        mov     bx,ax                   ; BL = R
        mov     al,ah                   ; DX:AX = T >> 8
        mov     ah,dl
        mov     dl,dh
        xor     dh,dh
        test    bl,80h
        jz      fd_pack
        test    bl,7Fh
        jnz     fd_up
        test    bp,bp
        jnz     fd_up
        test    al,1
        jz      fd_pack
fd_up:  add     ax,1
        adc     dx,0
fd_pack:
        dec     di
        mov     cl,7
        shl     di,cl
        add     dx,di
        or      dx,si
        mov     bx,dx
        and     bx,7F80h
        cmp     bx,7F80h
        jne     fd_ret
fd_inf: xor     ax,ax                   ; infinity
        mov     dx,si
        or      dx,7F80h
        jmp     fd_ret
fd_zero:
        xor     ax,ax
        mov     dx,si
        jmp     fd_ret
fd_spec:
        mov     bx,dx                   ; infinity or NaN
        and     bx,7FFFh
        or      bx,ax
        or      bx,bp
        jz      fd_inf
        mov     al,ah                   ; T >> 8, the fraction's top bits
        mov     ah,dl
        mov     dl,dh
        xor     dh,dh
        and     dx,007Fh
        or      dx,7FC0h                ; exponent 255, quiet bit
        or      dx,si
fd_ret: pop     bp
        pop     di
        pop     si
        ret
__FDFS  endp

        end
