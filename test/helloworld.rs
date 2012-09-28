#include <constants.rh>
#include <crctools.rh>
#include <math.rh>
#include <util.rh>
; vim: syntax=fasm


; Test RAR assembly file that just demonstrates the syntax.
;
; Usage:
;
;   $ unrar p -inul helloworld.rar
;   Hello, World!
;

_start:
    ; Install our message in the output buffer
    mov     r3, #0x1000                 ; Output buffer.
    mov     [r3+#0], #0x6c6c6548        ; 'lleH'
    mov     [r3+#4], #0x57202c6f        ; 'W ,o'
    mov     [r3+#8], #0x646c726f        ; 'dlro'
    mov     [r3+#12], #0x00000a21       ; '!\n'
    mov     [VMADDR_NEWBLOCKPOS],  r3   ; Pointer
    mov     [VMADDR_NEWBLOCKSIZE], #14  ; Size
    call    $_success
