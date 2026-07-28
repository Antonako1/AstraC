.use32
.code

_ADD:
    PUSH EBP
    MOV EBP, ESP
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP+12]
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    POP EBP
    RET
__ADD_ret:
    POP EBP
    RET

_SUBTRACT:
    PUSH EBP
    MOV EBP, ESP
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP+12]
    MOV EBX, EAX
    POP EAX
    SUB EAX, EBX
    POP EBP
    RET
__SUBTRACT_ret:
    POP EBP
    RET

_FACTORIAL:
    PUSH EBP
    MOV EBP, ESP
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, 0x1
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETLE AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl3
    MOV EAX, 0x1
    POP EBP
    RET
    JMP __lbl4
__lbl3:
__lbl4:
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, 0x1
    MOV EBX, EAX
    POP EAX
    SUB EAX, EBX
    PUSH EAX
    CALL _FACTORIAL
    ADD ESP, 4
    MOV EBX, EAX
    POP EAX
    IMUL EAX, EBX
    POP EBP
    RET
__FACTORIAL_ret:
    POP EBP
    RET

_RESET_COUNTERS:
    PUSH EBP
    MOV EBP, ESP
    MOV EAX, 0x0
    PUSH EAX
    POP EAX
    MOV [G_COUNTER], EAX
    MOV EAX, 0x0
    PUSH EAX
    POP EAX
    MOV [G_TICKS], EAX
    MOV EAX, 0
    PUSH EAX
    POP EAX
    MOV [G_READY], EAX
    POP EBP
    RET
__RESET_COUNTERS_ret:
    POP EBP
    RET

_APPLY_BINOP:
    PUSH EBP
    MOV EBP, ESP
    MOV EAX, [EBP+16]
    PUSH EAX
    MOV EAX, [EBP+12]
    PUSH EAX
    CALL [EBP+8]
    ADD ESP, 8
    POP EBP
    RET
__APPLY_BINOP_ret:
    POP EBP
    RET

_MUL_OP:
    PUSH EBP
    MOV EBP, ESP
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP+12]
    MOV EBX, EAX
    POP EAX
    IMUL EAX, EBX
    POP EBP
    RET
__MUL_OP_ret:
    POP EBP
    RET

_XOR_OP:
    PUSH EBP
    MOV EBP, ESP
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP+12]
    MOV EBX, EAX
    POP EAX
    XOR EAX, EBX
    POP EBP
    RET
__XOR_OP_ret:
    POP EBP
    RET

_SHL_OP:
    PUSH EBP
    MOV EBP, ESP
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP+12]
    MOV EBX, EAX
    POP EAX
    MOV ECX, EBX
    SHL EAX, CL
    POP EBP
    RET
__SHL_OP_ret:
    POP EBP
    RET

_MAIN:
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 217
    MOV EAX, 0xA
    MOV [EBP-0], EAX
    MOV EAX, 0x5
    NEG EAX
    MOV [EBP-4], EAX
    MOV EAX, 1
    MOV [EBP+8], EAX
    MOV EAX, _str2
    MOV [EBP+12], EAX
    MOV EAX, 0x41
    MOV [EBP+13], EAX
    MOV EAX, 0
    MOV [EBP+17], EAX
    MOV EAX, 0
    MOV [EBP+21], EAX
    MOV EAX, 0x64
    MOV [EBP+22], EAX
    MOV EAX, 0x3E8
    MOV [EBP+24], EAX
    MOV EAX, [EBP+22]
    MOV [EBP+28], EAX
    MOV EAX, 0xB8000
    MOV [EBP+32], EAX
    MOV EAX, [EBP+32]
    MOV [EBP+36], EAX
    MOV EAX, 0x1234
    MOV [EBP+37], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    PUSH EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    SUB EAX, EBX
    PUSH EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0x2
    MOV EBX, EAX
    POP EAX
    IMUL EAX, EBX
    MOV EBX, EAX
    POP EAX
    IMUL EAX, EBX
    PUSH EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0x3
    MOV EBX, EAX
    POP EAX
    XOR EDX, EDX
    DIV EBX
    MOV EAX, EDX
    MOV EBX, EAX
    POP EAX
    XOR EDX, EDX
    DIV EBX
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    MOV [EBP+41], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0xFF
    MOV EBX, EAX
    POP EAX
    AND EAX, EBX
    MOV [EBP+45], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0xFF00
    MOV EBX, EAX
    POP EAX
    OR EAX, EBX
    MOV [EBP+49], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0xAAAA
    MOV EBX, EAX
    POP EAX
    XOR EAX, EBX
    MOV [EBP+53], EAX
    MOV EAX, [EBP-0]
    NOT EAX
    MOV [EBP+57], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0x2
    MOV EBX, EAX
    POP EAX
    MOV ECX, EBX
    SHL EAX, CL
    MOV [EBP+61], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0x2
    MOV EBX, EAX
    POP EAX
    MOV ECX, EBX
    SHR EAX, CL
    MOV [EBP+65], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETE AL
    MOVZX EAX, AL
    MOV [EBP+69], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETNE AL
    MOVZX EAX, AL
    MOV [EBP+73], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETL AL
    MOVZX EAX, AL
    MOV [EBP+77], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETG AL
    MOVZX EAX, AL
    MOV [EBP+81], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETLE AL
    MOVZX EAX, AL
    MOV [EBP+85], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETGE AL
    MOVZX EAX, AL
    MOV [EBP+89], EAX
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0x0
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETG AL
    MOVZX EAX, AL
    MOV EBX, EAX
    POP EAX
    MOV [EBP+93], EAX
    MOV EAX, [EBP+8]
    PUSH EAX
    MOV EAX, [EBP-4]
    PUSH EAX
    MOV EAX, 0x0
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETL AL
    MOVZX EAX, AL
    MOV EBX, EAX
    POP EAX
    MOV [EBP+97], EAX
    MOV EAX, [EBP+8]
    TEST EAX, EAX
    SETE AL
    MOVZX EAX, AL
    MOV [EBP+101], EAX
    MOV EAX, 0x1
    MOV [EBP+105], EAX
    MOV EAX, 0x2
    MOV [EBP+109], EAX
    MOV EAX, 0x3
    MOV [EBP+113], EAX
    MOV EAX, 0x0
    PUSH EAX
    POP EAX
    MOV [EBP+113], EAX
    PUSH EAX
    POP EAX
    MOV [EBP+109], EAX
    PUSH EAX
    POP EAX
    MOV [EBP+105], EAX
    MOV EAX, 0xA
    MOV [EBP+117], EAX
    MOV EAX, 0x5
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x3
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x2
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x4
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x1
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x2
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x2
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x1
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x3
    PUSH EAX
    POP EAX
    MOV [EBP+117], EAX
    MOV EAX, 0x0
    MOV [EBP+121], EAX
    MOV EAX, 0x0
    MOV [EBP+125], EAX
    MOV EAX, [EBP+121]
    MOV [EBP+129], EAX
    XOR EAX, EAX
    MOV [EBP+133], EAX
    XOR EAX, EAX
    MOV [EBP+137], EAX
    MOV EAX, 0x64
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    MOV EAX, 0xC8
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    LEA EAX, [EBP+141]
    MOV [EBP+145], EAX
    MOV EAX, 0x12C
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    MOV EAX, 0x190
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    MOV EAX, 0x0
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    MOV EAX, 0x0
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    MOV EAX, 0x280
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    MOV EAX, 0x1E0
    PUSH EAX
    POP EAX
    XOR EAX, EAX
    MOV EAX, [EBP-0]
    MOV [EBP+153], EAX
    MOV EAX, [EBP+153]
    PUSH EAX
    MOV EAX, [EBP-0]
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETE AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl5
    MOV EAX, [EBP-0]
    PUSH EAX
    POP EAX
    MOV [EBP+153], EAX
    JMP __lbl6
__lbl5:
__lbl6:
    MOV EAX, [EBP-4]
    PUSH EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    CALL _ADD
    ADD ESP, 8
    MOV [EBP+157], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, [EBP-4]
    MOV EBX, EAX
    POP EAX
    PUSH EAX
    CALL _SUBTRACT
    ADD ESP, 4
    MOV [EBP+161], EAX
    MOV EAX, 0x5
    PUSH EAX
    CALL _FACTORIAL
    ADD ESP, 4
    MOV [EBP+165], EAX
    LEA EAX, [EBP-0]
    MOV [EBP+169], EAX
    MOV EAX, 0x1E
    PUSH EAX
    MOV EAX, 0x32
    PUSH EAX
    CALL [EBP+169]
    ADD ESP, 8
    MOV [EBP+173], EAX
    MOV EAX, 0x7
    PUSH EAX
    MOV EAX, 0x6
    PUSH EAX
    LEA EAX, [EBP-0]
    PUSH EAX
    CALL _APPLY_BINOP
    ADD ESP, 12
    MOV [EBP+177], EAX
    MOV EAX, 0xF
    PUSH EAX
    MOV EAX, 0xFF
    PUSH EAX
    LEA EAX, [EBP-0]
    PUSH EAX
    CALL _APPLY_BINOP
    ADD ESP, 12
    PUSH EAX
    POP EAX
    MOV [EBP+177], EAX
    MOV EAX, 0x5
    PUSH EAX
    MOV EAX, 0x1
    PUSH EAX
    LEA EAX, [EBP-0]
    PUSH EAX
    CALL _APPLY_BINOP
    ADD ESP, 12
    PUSH EAX
    POP EAX
    MOV [EBP+177], EAX
    MOV EAX, 0x0
    MOV [EBP+181], EAX
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0x64
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETG AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl7
    MOV EAX, 0x1
    PUSH EAX
    POP EAX
    MOV [EBP+181], EAX
    JMP __lbl8
__lbl7:
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0x32
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETG AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl9
    MOV EAX, 0x2
    PUSH EAX
    POP EAX
    MOV [EBP+181], EAX
    JMP __lbl10
__lbl9:
    MOV EAX, [EBP-0]
    PUSH EAX
    MOV EAX, 0xA
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETG AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl11
    MOV EAX, 0x3
    PUSH EAX
    POP EAX
    MOV [EBP+181], EAX
    JMP __lbl12
__lbl11:
    MOV EAX, 0x4
    PUSH EAX
    POP EAX
    MOV [EBP+181], EAX
    MOV EAX, 0x63
    MOV [EBP+185], EAX
    MOV EAX, [EBP+185]
    PUSH EAX
    POP EAX
    MOV [EBP+181], EAX
__lbl12:
__lbl10:
__lbl8:
    MOV EAX, 0x0
    MOV [EBP+189], EAX
    MOV EAX, 0x0
    MOV [EBP+193], EAX
__lbl13:
    MOV EAX, [EBP+193]
    PUSH EAX
    MOV EAX, 0xA
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETL AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl14
    MOV EAX, [EBP+193]
    PUSH EAX
    POP EAX
    MOV [EBP+189], EAX
    MOV EAX, [EBP+193]
    PUSH EAX
    MOV EAX, 0x5
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETE AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl16
    JMP __lbl13
    JMP __lbl17
__lbl16:
__lbl17:
    MOV EAX, [EBP+189]
    PUSH EAX
    MOV EAX, 0x1E
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETG AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl18
    JMP __lbl14
    JMP __lbl19
__lbl18:
__lbl19:
__lbl15:
    XOR EAX, EAX
    JMP __lbl13
__lbl14:
    MOV EAX, 0x5
    MOV [EBP+197], EAX
    MOV EAX, 0x0
    MOV [EBP+201], EAX
__lbl20:
    MOV EAX, [EBP+197]
    PUSH EAX
    MOV EAX, 0x0
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETG AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl21
    MOV EAX, [EBP+197]
    PUSH EAX
    POP EAX
    MOV [EBP+201], EAX
    XOR EAX, EAX
    JMP __lbl20
__lbl21:
    MOV EAX, 0x0
    MOV [EBP+205], EAX
    MOV EAX, 0x0
    MOV [EBP+209], EAX
__lbl22:
    MOV EAX, [EBP+205]
    PUSH EAX
    POP EAX
    MOV [EBP+209], EAX
    XOR EAX, EAX
    MOV EAX, [EBP+205]
    PUSH EAX
    MOV EAX, 0x5
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETL AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JNZ __lbl22
__lbl23:
    MOV EAX, 0x0
    MOV [EBP+213], EAX
    XOR EAX, EAX
TEST_LABEL:
    XOR EAX, EAX
    MOV EAX, [EBP+213]
    PUSH EAX
    MOV EAX, 0x3
    MOV EBX, EAX
    POP EAX
    CMP EAX, EBX
    SETL AL
    MOVZX EAX, AL
    TEST EAX, EAX
    JZ __lbl24
    JMP TEST_LABEL
    JMP __lbl25
__lbl24:
__lbl25:

MOV EAX, [EBP-0]
[EBP-0] EAX, [EBP-4]
MOV [EBP-0], EAX

    CALL _RESET_COUNTERS
    MOV EAX, [EBP+189]
    PUSH EAX
    MOV EAX, [EBP+201]
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    PUSH EAX
    MOV EAX, [EBP+209]
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    PUSH EAX
    MOV EAX, [EBP+213]
    MOV EBX, EAX
    POP EAX
    ADD EAX, EBX
    PUSH EAX
    POP EAX
    MOV [G_COUNTER], EAX
    MOV EAX, [G_COUNTER]
    PUSH EAX
    POP EAX
    MOV [G_TICKS], EAX
    MOV EAX, [G_COUNTER]
    POP EBP
    RET
__MAIN_ret:
    ADD ESP, 217
    POP EBP
    RET

.rodata
_str1 DB "AC Compiler Test", 0
_str2 DB "Hello", 0

.data
G_COUNTER DD 0
G_TICKS DD 0
G_MESSAGE DD 0
G_ORIGIN DD 0
G_COLOR DD 0
G_READY DD 0
