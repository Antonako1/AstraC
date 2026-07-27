; ==========================================================
; AstraC Assembler Test
; Exercises common assembler features
; ==========================================================

#define SCREEN 0xB8000

.use32

.data
counter     DD 0
value       DD 1234
numbers     DW 1, 2, 3, 4
message     DB "Assembler Test", 0
msg_len     DD $message

.rodata
magic       DW 0xABCD

.code

_start:
    PUSH EBP
    MOV EBP, ESP

    ; Register operations
    MOV EAX, 10
    MOV EBX, 20
    ADD EAX, EBX
    SUB EAX, 5
    XOR ECX, ECX
    INC ECX
    DEC ECX
    NEG EAX
    NOT EBX

    ; Memory access
    MOV [counter], EAX
    MOV EDX, [counter]

    ; Explicit operand sizes
    MOV BYTE PTR [SCREEN], 'A'
    MOV WORD PTR [SCREEN + 2], 0x0F42
    MOV DWORD PTR [value], 0x12345678

    ; Address calculation
    LEA ESI, [numbers]
    LEA EDI, [ESI + ECX*2 + 4]

    ; Conditional jump
    CMP EDX, 0
    JE zero
    JNE nonzero

zero:
    MOV EAX, 0
    JMP done

nonzero:
    MOV EAX, 1

done:
    CALL test_proc

    POP EBP
    RET


; ----------------------------------------------------------

test_proc:
    PUSH EAX
    PUSH EBX

    MOV ECX, 5

@@loop:
    DEC ECX
    JNZ @b

    POP EBX
    POP EAX
    RET


; ----------------------------------------------------------

.code

; Raw bytes
0x55
0xAA55
0xDEADBEEF