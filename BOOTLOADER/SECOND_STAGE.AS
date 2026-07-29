; BOOTLOADER/SECOND_STAGE.AS — Second stage
; Loads KERNEL from floppy sectors 10-18 to 0x10000,
; switches to 32-bit protected mode, and jumps to kernel.
.org 0x7E00
.use16
.code

    ; Read kernel (LBA 9-17, CHS 0/0/10-18) to 0x10000
    MOV AX, 0x1000      ; ES = 0x1000
    MOV ES, AX
    XOR BX, BX          ; ES:BX = 0x10000
    MOV CX, 0x000A      ; CH=0 (cylinder 0), CL=10 (sector 10)
    XOR DX, DX          ; DH=0 (head 0), DL=0 (drive A:)
    MOV AX, 0x0209      ; AH=0x02 (read), AL=9 (sectors)
    INT 0x13
    JC error

    ; Switch to 32-bit protected mode
    CLI

    ; Enable A20 gate via keyboard controller
    IN 0x92          ; AL = port 0x92
    OR AL, 2
    OUT 0x92         ; port 0x92 = AL

    ; Load GDT
    LGDT [gdt_desc]

    ; Set protected mode bit (hardcoded — assembler has issues with CR0 in .use16)
    DB 0x0F, 0x20, 0xC0     ; MOV EAX, CR0
    DB 0x83, 0xC8, 0x01     ; OR EAX, 1
    DB 0x0F, 0x22, 0xC0     ; MOV CR0, EAX

    ; Far jump to flush prefetch queue (named variables to work around bare DW/DD bug)
    DB 0xEA                 ; JMP FAR opcode
pm_entry_off DW pm_entry    ; offset (absolute address, GDT base=0)
pm_entry_sel DW 0x08        ; segment selector (code)

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

err_msg DB "SE", 0

; ── GDT (all DB to avoid DW/DD sizing bugs) ──────────────────────────
gdt_start:
    DB 0, 0, 0, 0, 0, 0, 0, 0   ; null descriptor
    DB 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00  ; code descriptor
    DB 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00  ; data descriptor
gdt_end:

gdt_desc:
gdt_limit DW gdt_end - gdt_start - 1 ; limit
gdt_base  DD gdt_start               ; base

; ── 32-bit entry ─────────────────────────────────────────────────────
.use32
pm_entry:
    MOV AX, 0x10        ; data segment selector
    MOV DS, AX
    MOV ES, AX
    MOV FS, AX
    MOV GS, AX
    MOV SS, AX
    MOV ESP, 0x90000    ; stack at 576 KB

    ; Jump to kernel (use register-indirect to avoid relative-offset bug)
    MOV EAX, 0x10000
    JMP EAX
