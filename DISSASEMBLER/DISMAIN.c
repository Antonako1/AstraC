/*
 * DISSASEMBLER/DISMAIN.c — Disassembler entry point (skeleton).
 *
 * Reads a binary file and emits a human-readable .DSM listing.
 * TODO: implement.
 */
#include "DISSASEMBLER.h"
#include "../ASSEMBLER/MNEMONICS.h"

BOOL DISASSEMBLE_FILE(FILE *input, FILE *output, ASTRAC_ARGS *cfg) {
    /* TODO */
    (void)input; (void)output; (void)cfg;
    return FALSE;
}

ASTRAC_RESULT START_DISSASEMBLER() {
    /* TODO */
    AC_PRINTF("[DISASM] Not yet implemented.\n");
    return ASTRAC_ERR_INTERNAL;
}
