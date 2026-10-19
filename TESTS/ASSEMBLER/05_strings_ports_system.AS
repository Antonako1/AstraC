; ==========================================================
; Test 05: String Instructions, Repeat Prefixes, Ports & System
; ==========================================================

.use32
.data
src_buf         DB "SourceString", 0
dst_buf         DB 0
gdt_desc        DW 0x3F, 0x0000, 0x0010
idt_desc        DW 0x7FF, 0x0000, 0x0020

.code
_start:
    ; Flag manipulation
    CLC
    STC
    CMC
    CLD
    STD
    CLI
    STI
    LAHF
    SAHF

    ; Stack register bulk operations
    PUSHA
    POPA
    PUSHAD
    POPAD
    PUSHF
    POPF
    PUSHFD
    POPFD

    ; String operations (bare)
    MOVSB
    MOVSW
    MOVSD
    LODSB
    LODSW
    LODSD
    STOSB
    STOSW
    STOSD
    CMPSB
    CMPSW
    CMPSD
    SCASB
    SCASW
    SCASD
    INSB
    INSW
    INSD
    OUTSB
    OUTSW
    OUTSD

    ; Repeat prefixes with string instructions
    REP MOVSB
    REP MOVSW
    REP MOVSD
    REP STOSB
    REP STOSD
    REPE CMPSB
    REPE CMPSD
    REPNE SCASB
    REPNE SCASD

    ; Port I/O: explicit operands
    IN  AL, 0x60
    OUT 0x60, AL
    IN  AX, 0x60
    OUT 0x60, AX
    IN  EAX, 0x60
    OUT 0x60, EAX

    IN  AL, DX
    OUT DX, AL
    IN  AX, DX
    OUT DX, AX
    IN  EAX, DX
    OUT DX, EAX

    ; Port I/O: legacy implicit forms
    IN  0x60
    OUT 0x60

    ; System instructions
    CPUID
    RDTSC
    LGDT [gdt_desc]
    LIDT [idt_desc]
    SGDT [gdt_desc]
    SIDT [idt_desc]

    ENTER 16, 0
    LEAVE

    ; Interrupts
    INT 0x20
    INT3
    INTO
    IRET

    NOP
    HLT
    RET
