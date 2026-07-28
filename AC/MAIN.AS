.use32
.code

_GET_STUDENT_NAME:
    PUSH EBP
    MOV EBP, ESP
    XOR EAX, EAX
    POP EBP
    RET
__GET_STUDENT_NAME_ret:
    POP EBP
    RET

_INCREMENT_STUDENT_ID:
    PUSH EBP
    MOV EBP, ESP
    XOR EAX, EAX
    XOR EAX, EAX
    POP EBP
    RET
__INCREMENT_STUDENT_ID_ret:
    POP EBP
    RET

_MAIN:
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 21
    MOV EAX, 0x3039
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    MOV EAX, _str1
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    LEA EAX, [EBP-0]
    MOV [EBP-4], EAX
    LEA EAX, [EBP-0]
    PUSH EAX
    CALL [EBP-4]
    ADD ESP, 4
    MOV EAX, 0x10
    MOV [EBP-5], EAX
    XOR EAX, EAX
    PUSH EAX
    MOV EAX, 0x3039
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETG AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl2
    MOV EAX, 0x0
    MOV [EBP+9], EAX
__lbl4:
    MOV EAX, [EBP+9]
    PUSH EAX
    MOV EAX, 0x5
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETL AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl5

MOV EAX, [EBP-0]
ADD EAX, [EBP+9]
MOV [EBP-0], EAX

__lbl6:
    XOR EAX, EAX
    JMP __lbl4
__lbl5:
    JMP __lbl3
__lbl2:
    MOV EAX, 0x0
    PUSH EAX
    POP EAX
    XOR EAX, EAX
__lbl3:
    XOR EAX, EAX
    MOV EAX, 0x0
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    JNE __lbl8
    PUSH EAX
    MOV EAX, 0x0
    POP EBP
    RET
    POP EAX
__lbl8:
    MOV EAX, 0x32
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    JNE __lbl9
    PUSH EAX
    MOV EAX, 0x32
    POP EBP
    RET
    POP EAX
__lbl9:
__lbl7:
    MOV EAX, 0x0
    MOV [EBP+13], EAX
__lbl10:
    MOV EAX, [EBP+13]
    PUSH EAX
    XOR EAX, EAX
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETL AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl11
    XOR EAX, EAX
    MOV EAX, [EBP+13]
    PUSH EAX
    MOV EAX, 0xA
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETE AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl12
    JMP __lbl11
    JMP __lbl13
__lbl12:
__lbl13:
    JMP __lbl10
__lbl11:
    LEA EAX, [EBP-0]
    PUSH EAX
    CALL _GET_STUDENT_NAME
    ADD ESP, 4
    MOV [EBP+17], EAX
    XOR EAX, EAX
    POP EBP
    RET
__MAIN_ret:
    ADD ESP, 21
    POP EBP
    RET

.rodata
_str1 DB "John Doe", 0
