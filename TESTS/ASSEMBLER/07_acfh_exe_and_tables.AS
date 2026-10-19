; ==========================================================
; Test 07: ACFH Executable Binary with Relocations & Import Table
; ==========================================================

.use32
.org 0x10000
.import "libc.lib" malloc, free
.import "math.lib" sin_f32

.data
val32: dd 0x12345678
ptr_to_msg: dd msg_hello

.rodata
msg_hello: db "Hello, ACFH!", 0

.bss
buffer: resb 1024
counter: resd 1

.code
_start:
    push ebp
    mov ebp, esp

    ; Reference data requiring relocation
    mov eax, [val32]
    mov edx, [ptr_to_msg]

    ; Dynamic library calls
    push 128
    call malloc
    add esp, 4
    mov ebx, eax

    push ebx
    call free
    add esp, 4

    ; Another call to test multiple patch offsets
    push 256
    call malloc
    add esp, 4

    push eax
    call free
    add esp, 4

    ; Internal function call
    call helper_func

    mov eax, 0
    pop ebp
    ret

helper_func:
    inc dword [val32]
    ret
