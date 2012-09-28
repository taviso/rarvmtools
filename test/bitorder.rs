#include <constants.rh>
#include <crctools.rh>
#include <math.rh>
#include <util.rh>
; vim: syntax=fasm

_start:
    push    #0x12345678
    call    $_invert_bit_order
    cmp     r0, #0x482c6a1e
    jz      $next
    call    $_error
next:
    push    r0
    call    $_invert_bit_order
    cmp     r0, #0x12345678
    jz      $finish
    call    $_error
finish:
    mov     [#0x1000], #0x000a4b4f
    mov     [VMADDR_NEWBLOCKPOS],  #0x1000   ; Pointer
    mov     [VMADDR_NEWBLOCKSIZE], #3        ; Size
    call    $_success
