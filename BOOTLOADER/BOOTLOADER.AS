; BOOTLOADER/BOOTLOADER.AS — First-stage bootloader
; Loads SECOND_STAGE from floppy sectors 2-9 and jumps to it.
.org 0x7C00
.use16
.code

    ; Set up segments
    XOR AX, AX
    MOV DS, AX
    MOV ES, AX
    MOV SS, AX
    MOV SP, 0x7C00

    ; Read second stage (LBA 1-8, CHS 0/0/2-9) to 0x7E00
    MOV BX, 0x7E00      ; ES:BX = 0x0000:0x7E00
    MOV CX, 0x0002      ; CH=0 (cylinder 0), CL=2 (sector 2)
    XOR DX, DX          ; DH=0 (head 0), DL=0 (drive A:)
    MOV AX, 0x0208      ; AH=0x02 (read), AL=8 (sectors)
    INT 0x13
    JC error

    ; Jump to second stage
    JMP 0x0000:0x7E00

error:
    MOV SI, err_msg
    MOV AH, 0x0E
.loop:
    LODSB
    OR AL, AL
    JZ .hang
    INT 0x10
    JMP .loop
.hang:
    HLT
    JMP .hang

err_msg DB "ERR", 0

.times 510-($-$$) DB 0
0xAA55
