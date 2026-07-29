.org 0x10000
.use32
.code

_MAIN:
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 4

; VGA TEXT MODE: EACH CELL IS CHAR (1B) + ATTR (1B), LITTLE-ENDIAN
MOV DWORD PTR [0XB8000], 0X04450448  ; "HE" WHITE ON BLACK
MOV DWORD PTR [0XB8004], 0X044C044C  ; "LL"
MOV DWORD PTR [0XB8008], 0X0420044F  ; "O "
MOV DWORD PTR [0XB800C], 0X044F0457  ; "WO"
MOV DWORD PTR [0XB8010], 0X044C0452  ; "RL"
MOV WORD  PTR [0XB8014], 0X0444      ; "D"

    CALL _CLEAR
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

_CLEAR:
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
__CLEAR_ret:
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
