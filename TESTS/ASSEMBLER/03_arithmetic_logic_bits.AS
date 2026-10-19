; ==========================================================
; Test 03: Arithmetic, Logic, Bit Operations, Shifts & Multiplies
; ==========================================================

.use32
.code

_start:
    ; Arithmetic: ADD, ADC, SUB, SBB, CMP, TEST
    ADD EAX, EBX
    ADD EAX, 100
    ADD DWORD PTR [ESI], 50
    ADC ECX, EDX
    ADC ECX, 1
    SUB EDX, EAX
    SUB EDX, 0x20
    SBB EBX, ECX
    CMP EAX, 0
    CMP EBX, EDX
    CMP BYTE PTR [EDI], 0x55
    TEST EAX, EAX
    TEST BL, 0x80

    ; Unary: INC, DEC, NEG, NOT
    INC EAX
    INC WORD PTR [ESI]
    DEC EDX
    DEC BYTE PTR [EDI]
    NEG EAX
    NOT EBX

    ; Logical: AND, OR, XOR
    AND EAX, 0xFF
    AND DWORD PTR [ESI], 0x00FF00FF
    OR  EBX, 0x80000000
    OR  WORD PTR [EDI], 0x0F0F
    XOR ECX, ECX
    XOR BYTE PTR [ESI], 0xFF

    ; Multiply and Divide
    MUL EBX                     ; EDX:EAX = EAX * EBX (unsigned)
    IMUL ECX                    ; EDX:EAX = EAX * ECX (signed 1-op)
    IMUL EAX, EBX               ; EAX = EAX * EBX (signed 2-op)
    IMUL EAX, EBX, 10           ; EAX = EBX * 10 (signed 3-op imm8)
    IMUL ESI, [EDI], 1000       ; ESI = [EDI] * 1000 (signed 3-op imm32)
    DIV EBX                     ; EAX = EDX:EAX / EBX, EDX = rem
    IDIV ECX                    ; signed divide

    ; Shifts and Rotates by 1, CL, imm8
    SHL EAX, 1
    SHL EAX, CL
    SHL EAX, 4
    SHR EBX, 1
    SHR EBX, CL
    SHR EBX, 8
    SAR EDX, 1
    SAR EDX, CL
    SAR EDX, 2
    ROL ECX, 1
    ROL ECX, CL
    ROL ECX, 3
    ROR ESI, 1
    ROR ESI, CL
    ROR ESI, 5
    RCL EDI, 1
    RCL EDI, CL
    RCL EDI, 4
    RCR EAX, 1
    RCR EAX, CL
    RCR EAX, 6

    ; Double-precision shifts
    SHLD EAX, EBX, 8
    SHLD EAX, EBX, CL
    SHRD EDX, ECX, 16
    SHRD EDX, ECX, CL

    ; Bit Scan & Bit Test
    BSF EAX, EBX
    BSR ECX, EDX
    BT  EAX, 5
    BT  [ESI], EBX
    BTS EDX, 3
    BTR ECX, 7
    BTC EAX, 1

    ; Zero and Sign Extension
    MOVZX EAX, BL
    MOVZX EAX, BYTE PTR [ESI]
    MOVZX EBX, DX
    MOVZX EBX, WORD PTR [EDI]
    MOVSX ECX, AL
    MOVSX ECX, BYTE PTR [ESI]
    MOVSX EDX, CX
    MOVSX EDX, WORD PTR [EDI]

    ; Conditional byte sets (SETcc)
    SETZ  AL
    SETNZ BL
    SETC  CL
    SETNC DL
    SETL  AH
    SETG  BH
    SETA  CH
    SETBE DH

    ; Exchange
    XCHG EAX, EBX
    XCHG [ESI], ECX
    XCHG AL, BL

    ; Conversions
    CBW
    CWDE
    CWD
    CDQ
    DAA
    DAS
    AAA
    AAS
    AAM 10
    AAD 10

    RET
