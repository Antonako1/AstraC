; BOOTLOADER/SECOND_STAGE.AS — Second stage
; Loads KERNEL from floppy sectors 10-18 to 0x10000,
; switches to 32-bit protected mode, and jumps to kernel.
.org 0x7E00
.use16
.code
main:
    ; Read kernel (LBA 9-17, CHS 0/0/10-18) to 0x10000
    MOV AX, 0x1000      ; ES = 0x1000
    MOV ES, AX
    XOR BX, BX          ; ES:BX = 0x10000
    MOV CX, 0x000A      ; CH=0 (cylinder 0), CL=10 (sector 10)
    XOR DX, DX          ; DH=0 (head 0), DL=0 (drive A:)
    MOV AX, 0x0212      ; AH=0x02 (read), AL=18 (sectors)
    INT 0x13
    JC error

    CLI


#define VBE 1
#ifdef VBE
    ; Check if VESA BIOS Extensions (VBE) are supported
    MOV AX, 0x4F00
    mov BX, 0x0000
    MOV ES, BX
    MOV DI, [bootstrap + 16]
    INT 0x10
    CMP AH, 0x00
    JNE error
    CMP AL, 0x4F
    JNE error 

    ; Switch to vga mode 0x13h (320x200 256-color)
    MOV AX, 0x4f02
    mov BX, 0x0013
    INT 0x10
    CMP AH, 0x00
    JNE error
    CMP AL, 0x4F
    JNE error

    ; Check if vga mode was set correctly
    MOV AX, 0x4f03
    INT 0x10
    CMP AH, 0x00
    JNE error
    CMP AL, 0x4F
    JNE error

    ; Store vga mode info in bootstrap structure
    MOV AX, 0x4F01
    MOV CX, 0x0013
    ; Store v
    MOV BX, 0x0000
    MOV ES, BX
    LEA DI, [bootstrap + 16 + 256] ; Offset to SVGA_MODE_INFO_REC in bootstrap structure
    INT 0x10
    CMP AH, 0x00
    JNE error
    CMP AL, 0x4F
    JNE error
    mov [bootstrap], AX

#endif ; VBE

    ; Switch to 32-bit protected mode

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

; Create a bootstrap structure to pass to the kernel
bootstrap DW 0x2BA
; _SVGA_INFO_REC
.times 256 DB 0 
; _SVGA_MODE_INFO_REC
.times 256 DB 0 

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


    ; Load bootstrap structure address into EDI
    MOV EDI, bootstrap

    ; Jump to kernel (use register-indirect to avoid relative-offset bug)
    MOV EAX, 0x10000
    JMP EAX
