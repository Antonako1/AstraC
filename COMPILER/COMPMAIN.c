/*
 * COMPILER/COMPMAIN.c — AC compiler pipeline orchestration (skeleton).
 *
 * Stages:
 *   1. PREPROCESS_C  — shared preprocessor (SHARED/PREPROCESS.c)
 *   2. COMP_LEX      — tokenise preprocessed source  (LEX.c)
 *   3. COMP_PARSE    — build AST                     (PARSER.c)
 *   4. COMP_VERIFY   — semantic checks               (VERIFY_AST.c)
 *   5. COMP_GEN      — emit x86-32 AS               (GEN.c)
 *
 * If -S (emit_asm_only) the pipeline stops after step 5.
 * If -E (PREPROCESS_ONLY) the pipeline stops after step 1.
 * TODO: implement.
 */
#include "COMPILER.h"
#include "../AstraC.h"
#include "../ASSEMBLER/ASSEMBLER.h"

U32 START_COMPILER() {
    /* TODO */
    AC_PRINTF("[COMP] Not yet implemented.\n");
    return ASTRAC_ERR_INTERNAL;
}
