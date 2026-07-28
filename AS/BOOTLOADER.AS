; Bootloader — prints "HELLO WORLD" to the screen via BIOS INT 0x10
.org 0x7C00
.use16
.code

    ; Initialise segment registers
    XOR AX, AX
    MOV DS, AX
    MOV ES, AX
    MOV SS, AX
    MOV SP, 0x7C00

    ; Print string via BIOS teletype
    LEA SI, msg
    MOV AH, 0x0E

.loop:
    LODSB
    OR AL, AL
    JZ .done
    INT 0x10
    JMP .loop

.done:
    HLT
    JMP .done

msg DB "HELLO WORLD", 0

.times 510-($-$$) DB 0
0xAA55
