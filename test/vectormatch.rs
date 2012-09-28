#include <constants.rh>
#include <crctools.rh>
#include <math.rh>
#include <util.rh>
; vim: syntax=fasm

_start:
    push    #0x12345678
    call    $_find_matching_vector
    cmp     r0, #0x85c34f1e
    jz      $next
    call    $_error
next:
    push    r0
    call    $_find_matching_vector
    cmp     r0, #0xdece37f9
    jz      $finish
    call    $_error
finish:
    mov     [#0x1000], #0x000a4b4f
    mov     [VMADDR_NEWBLOCKPOS],  #0x1000   ; Pointer
    mov     [VMADDR_NEWBLOCKSIZE], #3        ; Size
    call    $_success
