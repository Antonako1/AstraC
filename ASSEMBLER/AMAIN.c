/*
 * ASSEMBLER/AMAIN.c — Assembler pipeline orchestration (skeleton).
 *
 * Stages:
 *   1. PREPROCESS_ASM  — shared preprocessor
 *   2. LEX             — tokenise
 *   3. ASM_BUILD_AST   — build AST
 *   4. VERIFY_AST      — semantic checks
 *   5. OPTIMIZE        — peephole optimisation
 *   6. GEN_BINARY      — emit machine code
 */
#include "ASSEMBLER.h"

VOID DESTROY_ASM_INFO(PASM_INFO info) {
    if (!info) return;
    for (U32 i = 0; i < info->tmp_file_count; i++) {
        if (info->tmp_files[i]) AC_MFree(info->tmp_files[i]);
    }
    AC_MFree(info);
}

VOID DESTROY_TOK_ARR(ASM_TOK_ARRAY *toks) {
    if (!toks) return;
    for (U32 i = 0; i < toks->len; i++) {
        PASM_TOK tok = toks->toks[i];
        if (tok) {
            if (tok->txt) AC_MFree(tok->txt);
            AC_MFree(tok);
        }
    }
    AC_MFree(toks);
}

VOID LEX_DUMP(ASM_TOK_ARRAY *toks) {
    if (!toks) return;
    AC_PRINTF("[AS LEX] Dumping %u tokens:\n", toks->len);
    for (U32 i = 0; i < toks->len; i++) {
        PASM_TOK tok = toks->toks[i];
        if (!tok) continue;
        AC_PRINTF("  [%u:%u] %s: '%s' (val=%u)\n",
                  tok->line, tok->col,
                  TOKEN_TYPE_STR(tok->type),
                  tok->txt ? tok->txt : "(null)",
                  tok->num_val);
    }
}



ASTRAC_RESULT START_ASSEMBLING() {
    ASTRAC_ARGS  *cfg     = GET_ARGS();
    PASM_INFO     info    = NULLPTR;
    ASM_TOK_ARRAY *tok_arr = NULLPTR;
    ASM_AST_ARRAY *ast_arr = NULLPTR;
    ASTRAC_RESULT  result  = ASTRAC_OK;

    if (cfg->verbose) AC_PRINTF("[AS] Stage 1/6: Preprocessing...\n");
    info = PREPROCESS_ASM();
    if (!info || info->tmp_file_count == 0) {
        AC_PRINTF("[AS] Preprocessing failed.\n");
        result = ASTRAC_ERR_PREPROCESS;
        goto cleanup;
    }

    if (cfg->verbose) AC_PRINTF("[AS] Stage 2/6: Lexing...\n");
    tok_arr = LEX(info);
    if (!tok_arr || tok_arr->len == 0) {
        AC_PRINTF("[AS] Lexer produced no tokens, exiting.\n");
        result = ASTRAC_ERR_LEX;
        goto cleanup;
    }

    // LEX_DUMP(tok_arr);

    if (cfg->verbose) AC_PRINTF("[AS] Stage 3/6: Building AST...\n");
    ast_arr = ASM_BUILD_AST(tok_arr);
    if (!ast_arr) {
        AC_PRINTF("[AS] AST construction failed.\n");
        result = ASTRAC_ERR_AST;
        goto cleanup;
    }

    if (cfg->verbose) AC_PRINTF("[AS] Stage 4/6: Verifying AST...\n");
    if (!VERIFY_AST(ast_arr, info)) {
        AC_PRINTF("[AS] AST verification failed.\n");
        result = ASTRAC_ERR_VERIFY;
        goto cleanup;
    }

    if (cfg->verbose) AC_PRINTF("[AS] Stage 5/6: Optimizing...\n");
    if (!OPTIMIZE(ast_arr)) {
        AC_PRINTF("[AS] Optimization pass failed.\n");
        result = ASTRAC_ERR_OPTIMIZE;
        goto cleanup;
    }

    if (cfg->verbose) AC_PRINTF("[AS] Stage 6/6: Generating binary...\n");
    if (!GEN_BINARY(ast_arr, info)) {
        AC_PRINTF("[AS] Code generation failed.\n");
        result = ASTRAC_ERR_CODEGEN;
        goto cleanup;
    }
    
cleanup:
    DESTROY_AST_ARR(ast_arr);
    DESTROY_TOK_ARR(tok_arr);
    DESTROY_ASM_INFO(info);
    return result;
}
