; ==========================================================
; Test 02: Registers, ModR/M, SIB Addressing & Segment Overrides
; ==========================================================

.use32
.code

_start:
    ; Register-to-register moves
    MOV EAX, EBX
    MOV ECX, EDX
    MOV ESI, EDI
    MOV EBP, ESP
    MOV AX, BX
    MOV CX, DX
    MOV SI, DI
    MOV SP, BP
    MOV AL, BL
    MOV CL, DL
    MOV AH, BH
    MOV CH, DH

    ; Immediate to register
    MOV EAX, 0x12345678
    MOV AX, 0x1234
    MOV AL, 0x12

    ; Base-only memory
    MOV EAX, [EBX]
    MOV [ECX], EDX

    ; Base + 8-bit displacement
    MOV EAX, [EBX + 16]
    MOV [ECX - 8], EDX

    ; Base + 32-bit displacement
    MOV EAX, [EBX + 0x1000]
    MOV [ECX + 0x2000], EDX

    ; Base + Index (scale 1)
    MOV EAX, [ESI + EDI]
    MOV [EBX + ECX], EAX

    ; Base + Index * Scale
    MOV EAX, [ESI + EDI*2]
    MOV EDX, [ESI + EDI*4]
    MOV ECX, [ESI + EDI*8]

    ; Base + Index * Scale + Displacement
    MOV EAX, [ESI + EDI*4 + 32]
    MOV [EBX + ECX*8 - 16], EAX

    ; ESP base (forces SIB encoding)
    MOV EAX, [ESP]
    MOV EDX, [ESP + 8]
    MOV [ESP + 16], ECX

    ; EBP base (forces disp8/disp32 encoding)
    MOV EAX, [EBP]
    MOV EDX, [EBP - 4]
    MOV [EBP + 8], ECX

    ; Explicit size prefixes on memory operands
    MOV BYTE PTR [EAX], 0x55
    MOV WORD PTR [EBX], 0x1234
    MOV DWORD PTR [ECX], 0x12345678
    INC BYTE [EDX]
    DEC WORD [ESI]
    NOT DWORD [EDI]

    ; Segment register overrides
    MOV AX, [CS:EBX]
    MOV [ES:EDI], AL
    MOV [FS:EAX], EDX
    MOV [GS:ESI], ECX
    MOV [SS:EBP - 4], EBX

    ; Segment register instructions
    MOV DS, AX
    MOV AX, DS
    MOV ES, BX
    PUSH DS
    POP ES

    ; Control & Debug registers
    MOV EAX, CR0
    MOV CR0, EAX
    MOV EBX, DR7
    MOV DR7, EBX

    ; Load effective address (LEA)
    LEA EAX, [EBX + ECX*4 + 16]
    LEA ESI, [EDI + 100]

    RET
