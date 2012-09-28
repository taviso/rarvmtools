#include <constants.rh>
#include <util.rh>
#include <math.rh>
#include <crctools.rh>
; vim: syntax=fasm

; Test RAR assembly file that just demonstrates the syntax.

_start:
    ; Install our message in the output buffer
    mov     r3,       #0x1000            ; Output buffer.
    mov     [r3+#0],  #0x41414141        ; Padding for compensation
    mov     [r3+#4],  #0x0a414141        ; Padding for compensation
    mov     [r3+#8],  #0x6c6c6548        ; 'lleH'
    mov     [r3+#12], #0x57202c6f        ; 'W ,o'
    mov     [r3+#16], #0x646c726f        ; 'dlro'
    mov     [r3+#20], #0x00000a21        ; '!\n'
    mov     [VMADDR_NEWBLOCKPOS],  #0x00001000
    mov     [VMADDR_NEWBLOCKSIZE], #22

    ; Compensate to required CRC
    push    RAR_FILECRC
    push    [VMADDR_NEWBLOCKSIZE]
    push    [VMADDR_NEWBLOCKPOS]
    call    $_compensate_crc
    test    r0, r0
    jz      $finished
    call    $_error

finished:
    call    $_success
