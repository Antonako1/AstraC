/*
 * DISSASEMBLER/DISSASEMBLER.h — Disassembler interface.
 */
#ifndef DISSASEMBLER_H
#define DISSASEMBLER_H

#include "../AstraC.h"

typedef struct {
    PU8 buffer;
    U32 size;
    U32 offset;
    U32 base_addr;
} DISASM_CTX;

ASTRAC_RESULT START_DISSASEMBLER();
BOOL          DISASSEMBLE_FILE(FILE *input, FILE *output, ASTRAC_ARGS *cfg);

#endif /* DISSASEMBLER_H */
