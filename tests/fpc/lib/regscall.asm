; regscall.asm - call one floating-point helper of docs/spec/fpc.md with
; chosen register contents, and report the registers afterwards, for
; tests/fpc/regs.c. Test code, not part of the library.
;
;   void __cdecl fpc_call(struct blk *p);
;
; p points into DGROUP (only its offset is used; SS = DGROUP is checked).
; The block is an array of words (the layout is struct blk in regs.c):
;
;   +0    op        which helper (table at TBL)
;   +2    AX BX CX DX before the call
;   +16   operand slot 0: 8 bytes at SS:SI for the F forms
;   +32   operand slot 1: 8 bytes at ES:SI for the E forms
;   +40   after the call: AX BX CX DX SI DI BP DS ES SS SP flags (12 words)
;   +64   1 if the stack below the helper's 64 bytes was untouched
;   +66   0, or 2 if SS is not DS on entry (nothing was called)
;   +68   markers for DI, BP and DS before the call; DF (0 or 1)
;   +76   written here before the call: SI, ES, SS, SP as they are then
;
; Before the call: DS = the marker (another segment), SI = the offset of
; slot 0, ES = DGROUP + 1 paragraph, so that ES:SI is slot 1 (16 bytes
; above slot 0, which SS:SI is), interrupts are off and the 192 bytes below
; SP hold a pattern. Whatever the helper reads through the wrong pair of
; registers is a decoy. The registers after the call are kept in the code
; segment, so that nothing depends on a register the helper might change.

        .8086
        include model.inc

ifdef __LARGE__
        .model large
elseifdef __COMPACT__
        .model compact
else
        .model small
endif

OP      equ     0
IN_AX   equ     2
IN_BX   equ     4
IN_CX   equ     6
IN_DX   equ     8
SLOT0   equ     16
OUTR    equ     40
GUARD   equ     64
STATUS  equ     66
MDI     equ     68
MBP     equ     70
MDS     equ     72
MDF     equ     74
ENT_SI     equ     76
ENT_ES     equ     78
ENT_SS     equ     80
ENT_SP     equ     82

PAT     equ     192             ; bytes below SP filled with the pattern
LIVE    equ     72              ; of which the top 72 are not checked
                                ; (64 bytes of stack and the return address)

ifdef FARCODE
TPTR    textequ <dd>
else
TPTR    textequ <dw>
endif

        .code

        extrn   __FDA:CDIST
        extrn   __FDS:CDIST
        extrn   __FDM:CDIST
        extrn   __FDD:CDIST
        extrn   __FDC:CDIST
        extrn   __FDN:CDIST
        extrn   __FDI4:CDIST
        extrn   __FDU4:CDIST
        extrn   __I4FD:CDIST
        extrn   __U4FD:CDIST
        extrn   __FDFS:CDIST
        extrn   __FSFD:CDIST
ifdef FARDATA
        extrn   __EDA:CDIST
        extrn   __EDS:CDIST
        extrn   __EDM:CDIST
        extrn   __EDD:CDIST
        extrn   __EDC:CDIST
endif

        public  _fpc_call

tbl     label   word
        TPTR    __FDA
        TPTR    __FDS
        TPTR    __FDM
        TPTR    __FDD
        TPTR    __FDC
        TPTR    __FDN
        TPTR    __FDI4
        TPTR    __FDU4
        TPTR    __I4FD
        TPTR    __U4FD
        TPTR    __FDFS
        TPTR    __FSFD
ifdef FARDATA
        TPTR    __EDA
        TPTR    __EDS
        TPTR    __EDM
        TPTR    __EDD
        TPTR    __EDC
endif

sv_blk  dw      0
sv_sp   dw      0
sv_ss   dw      0
sv_tgt  dd      0
rec     label   word
rec_ax  dw      0
rec_bx  dw      0
rec_cx  dw      0
rec_dx  dw      0
rec_si  dw      0
rec_di  dw      0
rec_bp  dw      0
rec_ds  dw      0
rec_es  dw      0
rec_ss  dw      0
rec_sp  dw      0
rec_fl  dw      0

_fpc_call proc  CDIST
        push    bp
        mov     bp,sp
        push    si
        push    di
        push    ds
        push    es
        pushf
        mov     bx,[bp+ARG0]
        mov     ax,ss
        mov     cx,ds
        cmp     ax,cx
        je      ssok
        mov     word ptr [bx+STATUS],2
        jmp     done
ssok:
        mov     word ptr [bx+STATUS],0
        mov     cs:sv_blk,bx
        mov     cs:sv_sp,sp
        mov     cs:sv_ss,ss
        lea     si,[bx+SLOT0]
        mov     [bx+ENT_SI],si
        mov     ax,ds
        inc     ax
        mov     [bx+ENT_ES],ax
        mov     [bx+ENT_SS],ss
        mov     [bx+ENT_SP],sp
        mov     si,[bx+OP]
        shl     si,1
ifdef FARCODE
        shl     si,1
endif
        mov     ax,cs:tbl[si]
        mov     word ptr cs:sv_tgt,ax
ifdef FARCODE
        mov     ax,cs:tbl[si+2]
        mov     word ptr cs:sv_tgt+2,ax
endif
        cli
        cld
        mov     ax,ss
        mov     es,ax
        mov     di,sp
        sub     di,PAT
        mov     cx,PAT/2
        mov     ax,0A5A5h
        rep     stosw
        cmp     word ptr [bx+MDF],0
        je      nodf
        std
nodf:
        mov     di,[bx+MDI]
        mov     bp,[bx+MBP]
        lea     si,[bx+SLOT0]
        mov     ax,ds
        inc     ax
        mov     es,ax
        mov     ax,[bx+MDS]
        mov     ds,ax
        mov     ax,ss:[bx+IN_AX]
        mov     cx,ss:[bx+IN_CX]
        mov     dx,ss:[bx+IN_DX]
        mov     bx,ss:[bx+IN_BX]
ifdef FARCODE
        call    dword ptr cs:sv_tgt
else
        call    word ptr cs:sv_tgt
endif
        pushf
        pop     word ptr cs:rec_fl
        mov     cs:rec_ax,ax
        mov     cs:rec_bx,bx
        mov     cs:rec_cx,cx
        mov     cs:rec_dx,dx
        mov     cs:rec_si,si
        mov     cs:rec_di,di
        mov     cs:rec_bp,bp
        mov     ax,ds
        mov     cs:rec_ds,ax
        mov     ax,es
        mov     cs:rec_es,ax
        mov     ax,ss
        mov     cs:rec_ss,ax
        mov     cs:rec_sp,sp
        mov     ax,cs:sv_ss
        mov     ss,ax
        mov     sp,cs:sv_sp
        mov     ax,ss
        mov     ds,ax
        mov     es,ax
        cld
        mov     di,sp
        sub     di,PAT
        mov     cx,(PAT-LIVE)/2
        mov     ax,0A5A5h
        repe    scasw
        mov     ax,1
        je      gdone
        mov     ax,0
gdone:
        mov     bx,cs:sv_blk
        mov     [bx+GUARD],ax
        push    cs
        pop     ds
        mov     si,offset rec
        lea     di,[bx+OUTR]
        mov     cx,12
        rep     movsw
done:
        popf
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     bp
        ret
_fpc_call endp

        end
