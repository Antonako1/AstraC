/*
 * ASSEMBLER/GEN.c — Binary code generator (skeleton).
 *
 * Two-pass emit: first pass resolves label offsets, second pass
 * writes final machine code with resolved relocations.
 * Output format: AC_FILE_HEADER followed by code/data/rodata sections.
 * TODO: implement.
 */
#include "ASSEMBLER.h"
#include "../AC_FH.h"

BOOLEAN GEN_BINARY(ASM_AST_ARRAY *ast, PASM_INFO info) {
    /* TODO */
    (void)ast; (void)info;
    return FALSE;
}
