#include <constants.rh>
#include <crctools.rh>
#include <math.rh>
#include <util.rh>
; vim: syntax=fasm

_start:
    mov     r3, #0x1000             ; Output buffer address
    mov     [r3], #0x6c6c6548       ; 'lleH'
    mov     [r3+#4], #0x57202c6f    ; 'W ,o'
    mov     [r3+#8], #0x646c726f    ; 'dlro'
    mov     [r3+#12], #0x00000a21   ; '!\n'
    push    #14                     ; Length
    push    r3                      ; Pointer
    call    $_crc_block
    cmp     r0, #0x4b17617b         ; Check value
    jz      $next
    call    $_error
next:
    push    #0xDEADBEEF
    push    #14
    push    #0x1000
    call    $_compensate_crc
    test    r0, r0
    jz      $verify
    call    $_error
verify:
    push    #14
    push    #0x1000
    call    $_crc_block
    cmp     r0, #0xDEADBEEF
    jz      $finish
    call    $_error

finish:
    mov     [#0x1000], #0x000a4b4f
    mov     [VMADDR_NEWBLOCKPOS],  #0x1000   ; Pointer
    mov     [VMADDR_NEWBLOCKSIZE], #3        ; Size
    call    $_success
