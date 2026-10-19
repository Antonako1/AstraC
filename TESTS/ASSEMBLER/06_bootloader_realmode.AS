; ==========================================================
; Test 06: 16-Bit Real Mode Bootloader, 16-bit ModR/M & Padding
; ==========================================================

.use16
.org 0x7C00
.code

_start:
    CLI
    XOR AX, AX
    MOV DS, AX
    MOV ES, AX
    MOV SS, AX
    MOV SP, 0x7C00
    STI

    ; 16-bit memory addressing forms
    MOV AX, [BX + SI]
    MOV BX, [BX + DI]
    MOV CX, [BP + SI]
    MOV DX, [BP + DI]
    MOV SI, [SI]
    MOV DI, [DI]
    MOV AX, [BP + 4]
    MOV BX, [BX - 2]
    MOV CX, [0x1234]

    ; Real-mode string print loop
    LEA SI, msg
    CALL print_str

    HLT

print_str:
    LODSB
    OR AL, AL
    JZ .done
    MOV AH, 0x0E
    INT 0x10
    JMP print_str
.done:
    RET

msg DB "AstraC Real-Mode Boot Sector", 0

; Pad to 510 bytes from section start, followed by MBR signature 0xAA55
.times 510-($-$$) db 0
0xAA55
