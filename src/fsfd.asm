; fsfd.asm - __FSFD: (double)f for float f.  DX:AX in, AX:BX:CX:DX out.
; Exact.  Docs: docs/spec/fpc.md.
;
; Without the sign, the float is u = e8 << 23 | m, and the double is the
; 64-bit (u << 29) plus (896 << 52), which turns the exponent bias 127
; into 1023: u as the high 32 bits, shifted right by 3.  Special cases:
;   - zero: zero with the sign
;   - e8 = 255 (infinity, NaN): the exponent adds 1792 << 52 instead, so
;     that it is 7FFh; a NaN gets its quiet bit set
;   - subnormal: normalise u until bit 23 is set (as if e8 = 1), each
;     shift taking 1 from the exponent.
; SI = the sign, DI = what is added to the high word of the result.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

        public  __FSFD

        .code

__FSFD  proc CDIST
        push    si
        push    di
        mov     si,dx
        and     si,8000h
        and     dx,7FFFh
        mov     di,dx
        or      di,ax
        jnz     fs_nz
        mov     ax,si                   ; zero
        xor     bx,bx
        xor     cx,cx
        xor     dx,dx
        jmp     fs_ret
fs_nz:  mov     bx,ax                   ; AX:BX = u, the high 32 bits of 64
        mov     ax,dx
        xor     cx,cx
        xor     dx,dx
        mov     di,3800h                ; 896 << 4
        cmp     ax,7F80h
        jb      fs_chk
        mov     di,7000h                ; infinity or NaN: 1792 << 4
        test    ax,007Fh
        jnz     fs_nan
        test    bx,bx
        jz      fs_sh
fs_nan: or      ax,0040h                ; quiet bit
        jmp     fs_sh
fs_chk: cmp     ax,0080h                ; bit 23 clear: subnormal
        jae     fs_sh
        shl     bx,1
        rcl     ax,1
        sub     di,16
        jmp     fs_chk
fs_sh:  rept    3
        shr     ax,1
        rcr     bx,1
        rcr     cx,1
        rcr     dx,1
        endm
        add     ax,di
        or      ax,si
fs_ret: pop     di
        pop     si
        ret
__FSFD  endp

        end
