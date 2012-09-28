#include <constants.rh>
#include <crctools.rh>
#include <math.rh>
#include <util.rh>
; vim: syntax=fasm

_start:
    mov     r0, #0x12345678
    mov     r1, r0
    cmp     r1, #0x12345678
    jnz     $failure
    xor     r0, r0
    mov     [r0], #0x41414141
    mov     r1, [r0]
    cmp     r1, #0x41414141
    jnz     $failure
    mov     [r0+#33], #0x41414141
    mov     r1, [r0+#33]
    cmp     r1, #0x41414141
    jnz     $failure
    mov     [#0x22], #0x42424242
    mov     r1, [#0x22]
    cmp     r1, #0x42424242
    jnz     $failure
    mov     [#0x44], [#0x22]
    mov     r1, [#0x44]
    jnz     $failure
    jmp     $finish

failure:
    call    $_error

finish:
    mov     [#0x1000], #0x000a4b4f
    mov     [VMADDR_NEWBLOCKPOS],  #0x1000   ; Pointer
    mov     [VMADDR_NEWBLOCKSIZE], #3        ; Size
    call    $_success
