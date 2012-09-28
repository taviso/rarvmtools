#include <constants.rh>
#include <crctools.rh>
#include <math.rh>
#include <util.rh>
; vim: syntax=fasm

_start:
    push    #32
    push    #0x12345678
    call    $_mod
    cmp     r0, #24
    jz      $next
    call    $_error
next:
    push    #6
    push    #0x41414141
    call    $_mod
    cmp     r0, #5
    jz      $finish
    call    $_error
finish:
    mov     [#0x1000], #0x000a4b4f
    mov     [VMADDR_NEWBLOCKPOS],  #0x1000   ; Pointer
    mov     [VMADDR_NEWBLOCKSIZE], #3        ; Size
    call    $_success
