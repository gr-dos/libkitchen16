; fdfromi.asm - __I4FD and __U4FD: (double)l for long l, and for unsigned
; long.  DX:AX in, AX:BX:CX:DX out.  Always exact.  Docs: docs/spec/fpc.md.
;
; The magnitude u (negated first if l < 0) is shifted left until bit 31 is
; set, n places; then the double is u * 2^21 with its hidden bit at bit 52
; plus (1053 - n) << 52 (1023 + 31 = 1054 for the exponent, less 1 for the
; hidden bit).  Zero is +0.  SI = the sign, DI = the exponent word.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __I4FD
        public  __U4FD

        .code

__U4FD  proc CDIST
        push    si
        push    di
        xor     si,si
        jmp     fl_body
__U4FD  endp

__I4FD  proc CDIST
        push    si
        push    di
        xor     si,si
        test    dx,dx
        jns     fl_body
        neg     dx                      ; magnitude
        neg     ax
        sbb     dx,0
        mov     si,8000h
        jmp     fl_body
__I4FD  endp

fl_body proc CDIST
        mov     di,dx
        or      di,ax
        jnz     fl_nz
        xor     bx,bx                   ; zero
        xor     cx,cx
        jmp     fl_ret
fl_nz:  mov     di,41D0h                ; 1053 << 4
fl_n:   test    dx,dx                   ; normalise
        js      fl_nd
        jnz     fl_nb
        mov     dx,ax                   ; high word zero: 16 places
        xor     ax,ax
        sub     di,256
        jmp     fl_n
fl_nb:  shl     ax,1
        rcl     dx,1
        sub     di,16
        jmp     fl_n
fl_nd:  mov     bx,ax                   ; AX:BX:CX:DX = (u << 32) >> 11
        mov     ax,dx
        xor     cx,cx
        xor     dx,dx
        push    bp
        mov     bp,11
fl_s:   shr     ax,1
        rcr     bx,1
        rcr     cx,1
        rcr     dx,1
        dec     bp
        jnz     fl_s
        pop     bp
        add     ax,di
        or      ax,si
fl_ret: pop     di
        pop     si
        ret
fl_body endp

        end
