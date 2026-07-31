.org 0x10000
.use32
.code

_MAIN:
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 4
    CALL _CLEAR_SCREEN
    MOV EAX, _str1
    MOV [EBP-4], EAX
    MOV EAX, [EBP-4]
    PUSH EAX
    CALL _PRINT_S
    ADD ESP, 4
__lbl2:
    MOV EAX, 0x1
    TEST EAX, EAX
    JZ __lbl3
 HLT 
    JMP __lbl2
__lbl3:
__MAIN_ret:
    ADD ESP, 4
    POP EBP
    RET

_CLEAR_SCREEN:
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 12
    MOV EAX, 0xB8000
    MOV [EBP-4], EAX
    MOV EAX, 0x0
    MOV [EBP-8], EAX
    MOV EAX, 0x0
    MOV [EBP-12], EAX
__lbl4:
    MOV EAX, [EBP-12]
    PUSH EAX
    MOV EAX, 0x50
    PUSH EAX
    MOV EAX, 0x19
    MOV EBX, EAX
    POP EAX
    IMUL EAX, EBX
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETL AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl5
    MOV EAX, 0x20
    PUSH EAX
    MOV EAX, [EBP-4]
    PUSH EAX
    MOV EAX, [EBP-8]
    POP EBX
    LEA EAX, [EBX + EAX]
    POP EBX
    MOV [EAX], BL
    MOV EAX, 0x7
    PUSH EAX
    MOV EAX, [EBP-4]
    PUSH EAX
    MOV EAX, [EBP-8]
    PUSH EAX
    MOV EAX, 0x1
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    POP EBX
    LEA EAX, [EBX + EAX]
    POP EBX
    MOV [EAX], BL
    MOV EAX, [EBP-8]
    PUSH EAX
    MOV EAX, 0x2
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    PUSH EAX
    POP EAX
    MOV [EBP-8], EAX
__lbl6:
    MOV EAX, [EBP-12]
    PUSH EAX
    ADD EAX, 1
    MOV [EBP-12], EAX
    POP EAX
    JMP __lbl4
__lbl5:
__CLEAR_SCREEN_ret:
    ADD ESP, 12
    POP EBP
    RET

_PRINT_S:
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 12
    MOV EAX, 0xB8000
    MOV [EBP-4], EAX
    MOV EAX, 0x0
    MOV [EBP-8], EAX
    MOV EAX, 0x0
    MOV [EBP-12], EAX
__lbl7:
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP-12]
    POP EBX
    MOVZX EAX, BYTE [EBX + EAX]
    PUSH EAX
    MOV EAX, 0x0
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETNE AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl8
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP-12]
    POP EBX
    MOVZX EAX, BYTE [EBX + EAX]
    PUSH EAX
    MOV EAX, [EBP-4]
    PUSH EAX
    MOV EAX, [EBP-8]
    POP EBX
    LEA EAX, [EBX + EAX]
    POP EBX
    MOV [EAX], BL
    MOV EAX, 0x2
    PUSH EAX
    MOV EAX, [EBP-4]
    PUSH EAX
    MOV EAX, [EBP-8]
    PUSH EAX
    MOV EAX, 0x1
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    POP EBX
    LEA EAX, [EBX + EAX]
    POP EBX
    MOV [EAX], BL
    MOV EAX, [EBP-8]
    PUSH EAX
    MOV EAX, 0x2
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    PUSH EAX
    POP EAX
    MOV [EBP-8], EAX
__lbl9:
    MOV EAX, [EBP-12]
    PUSH EAX
    MOV EAX, 0x1
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    PUSH EAX
    POP EAX
    MOV [EBP-12], EAX
    JMP __lbl7
__lbl8:
__PRINT_S_ret:
    ADD ESP, 12
    POP EBP
    RET

_PRINT_C:
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 4
    MOV EAX, 0xB8000
    MOV [EBP-4], EAX
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP-4]
    PUSH EAX
    MOV EAX, 0x0
    POP EBX
    LEA EAX, [EBX + EAX]
    POP EBX
    MOV [EAX], BL
    MOV EAX, 0x7
    PUSH EAX
    MOV EAX, [EBP-4]
    PUSH EAX
    MOV EAX, 0x1
    POP EBX
    LEA EAX, [EBX + EAX]
    POP EBX
    MOV [EAX], BL
__PRINT_C_ret:
    ADD ESP, 4
    POP EBP
    RET

.rodata
_str1 DB "Hello, World!", 0
