; ==========================================================
; Test 04: Control Flow, Local Labels, Relaxation & Loops
; ==========================================================

.use32
.data
target_ptr      DD sub_routine
jump_table      DD target1, target2, target3

.code
_start:
    ; Global routine with local labels
    MOV ECX, 3

@@outer:
    DEC ECX
    JZ @f                       ; Jump forward to next local label @@done
    MOV EBX, 2

@@inner:
    DEC EBX
    JNZ @b                      ; Jump backward to @@inner
    JMP @@outer                 ; Jump backward by name

@@done:
    ; Unconditional jumps
    JMP SHORT near_target
    JMP NEAR far_target

near_target:
    JMP far_target

far_target:
    ; Indirect jumps and calls
    MOV EAX, sub_routine
    CALL EAX
    CALL [target_ptr]
    CALL sub_routine

    ; Conditional jumps (all conditions)
    CMP EAX, 0
    JZ   cond_jz
    JNZ  cond_jnz
cond_jz:
cond_jnz:
    JC   cond_jc
    JNC  cond_jnc
cond_jc:
cond_jnc:
    JA   cond_ja
    JAE  cond_jae
cond_ja:
cond_jae:
    JB   cond_jb
    JBE  cond_jbe
cond_jb:
cond_jbe:
    JL   cond_jl
    JLE  cond_jle
cond_jl:
cond_jle:
    JG   cond_jg
    JGE  cond_jge
cond_jg:
cond_jge:
    JS   cond_js
    JNS  cond_jns
cond_js:
cond_jns:
    JO   cond_jo
    JNO  cond_jno
cond_jo:
cond_jno:
    JP   cond_jp
    JNP  cond_jnp
cond_jp:
cond_jnp:

    ; Automatic Jump Relaxation test:
    ; Jump over 200 bytes to force relaxation from rel8 to rel32 (0F 84)
    CMP EAX, 42
    JZ relaxed_target
    .times 200 db 0x90
relaxed_target:

    ; Loop instructions
    MOV ECX, 5
@@loop_loop:
    LOOP @@loop_loop

    MOV ECX, 5
@@loop_loope:
    LOOPE @@loop_loope

    MOV ECX, 5
@@loop_loopne:
    LOOPNE @@loop_loopne

    JECXZ @f
    NOP
@@skip:

    RET

; ----------------------------------------------------------
; Second global subroutine with its own @@loop local label scope
; ----------------------------------------------------------
sub_routine:
    PUSH EBP
    MOV EBP, ESP
    MOV ECX, 4
@@loop:
    DEC ECX
    JNZ @b                      ; Scoped to sub_routine.@@loop without collision
    POP EBP
    RET 4

target1:
    NOP
    RET
target2:
    NOP
    RET
target3:
    NOP
    RET
