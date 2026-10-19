; ==========================================================
; Test 01: Data Sections, Variables, Types and Literals
; ==========================================================

#define ITEM_COUNT 4

.data
str1            DB "Hello, world!\n\r\t\0\\\"", 0
str1_len        DD $str1
byte_val        DB 0x2A
word_val        DW 0x1234
dword_val       DD 0x12345678
word_neg        DW -1
dword_neg       DD -100
num_list        DW 10, 20, 30, 40
num_list_len    DD $num_list
pi_float        REAL4 3.14

.rodata
magic_rodata    DD 0xCAFEBABE
ro_string       DB "Read-Only Text", 0

.bss
bss_val1        DD 0
bss_val2        DW 0
bss_buf         DB 0
.times 128 db 0

.code
.use32

_start:
    MOV AL, [byte_val]
    MOV AX, [word_val]
    MOV EAX, [dword_val]
    MOV CX, [word_neg]
    MOV EDX, [dword_neg]
    MOV ESI, [str1_len]
    MOV EDI, [num_list_len]
    MOV EBX, [magic_rodata]

    ; Bare numbers in code section
    0x90
    0xAA55
    0xDEADBEEF
    RET
