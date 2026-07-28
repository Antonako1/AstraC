/*
 * COMPILER/COMPMAIN.c — AC compiler pipeline orchestration.
 *
 * Stages:
 *   1. PREPROCESS_C  — shared preprocessor (SHARED/PREPROCESS.c)
 *   2. COMP_LEX      — tokenise preprocessed source  (LEX.c)
 *   3. COMP_PARSE    — build AST                     (PARSER.c)
 *   4. COMP_VERIFY   — semantic checks               (VERIFY_AST.c)
 *   5. COMP_GEN      — emit x86 AS                   (GEN.c)
 *
 * If -S (emit_asm_only) the pipeline stops after step 5.
 * If -E (PREPROCESS_ONLY) the pipeline stops after step 1.
 */
#include "COMPILER.h"
#include "../AstraC.h"
#include "../ASSEMBLER/ASSEMBLER.h"

U32 START_COMPILER() {
    ASTRAC_ARGS *cfg = GET_ARGS();
    if (!cfg || !cfg->input_file) {
        AC_PRINTF("[COMP] Error: no input file\n");
        return ASTRAC_ERR_INTERNAL;
    }

    COMP_CTX *comp_ctx = (COMP_CTX *)AC_MAlloc(sizeof(COMP_CTX));
    if (!comp_ctx) { AC_PRINTF("[COMP] Out of memory\n"); return ASTRAC_ERR_INTERNAL; }
    AC_MEMZERO(comp_ctx, sizeof(COMP_CTX));
    comp_ctx->verbose = cfg->verbose;

    /* ── Stage 1: Preprocessing ────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 1/5: Preprocessing...\n");
    #ifdef _WIN32
    PU8 tmp_src_path = (PU8)"C:\\TMP\\00.AC";
    #else
    PU8 tmp_src_path = (PU8)"/TMP/00.AC";
    #endif

    PASM_INFO info = PREPROCESS_C();
    if (!info) {
        AC_PRINTF("[COMP] Preprocessor failed\n");
        AC_MFree(comp_ctx);
        return ASTRAC_ERR_PREPROCESS;
    }

    if (cfg->stepoff_level == 1 || cfg->build_type == BUILD_TYPE_PREPROCESS_ONLY) {
        AC_PRINTF("[COMP] Preprocess-only: output in %s\n", tmp_src_path);
        AC_MFree(comp_ctx);
        return ASTRAC_OK;
    }

    comp_ctx->tmp_src = NULLPTR;

    /* Read preprocessed source into memory */
    {
        FILE *pf = fopen(tmp_src_path, "rb");
        if (!pf) {
            AC_PRINTF("[COMP] Cannot open preprocessed file: %s\n", tmp_src_path);
            AC_MFree(comp_ctx);
            return ASTRAC_ERR_PREPROCESS;
        }
        fseek(pf, 0, SEEK_END);
        U32 psize = (U32)ftell(pf);
        fseek(pf, 0, SEEK_SET);
        comp_ctx->tmp_src = (PU8)AC_MAlloc(psize + 1);
        if (!comp_ctx->tmp_src) {
            fclose(pf); AC_MFree(comp_ctx);
            return ASTRAC_ERR_INTERNAL;
        }
        fread(comp_ctx->tmp_src, 1, psize, pf);
        comp_ctx->tmp_src[psize] = '\0';
        fclose(pf);
    }

    /* ── Stage 2: Lex ──────────────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 2/5: Lexing...\n");
    PCOMP_TOK_ARRAY toks = COMP_LEX(comp_ctx);
    if (!toks) {
        AC_PRINTF("[COMP] Lexer failed\n");
        AC_MFree(comp_ctx);
        return ASTRAC_ERR_LEX;
    }

    /* ── Stage 3: Parse ─────────────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 3/5: Parsing...\n");
    PCNODE ast = COMP_PARSE(toks, comp_ctx);
    if (!ast) {
        AC_PRINTF("[COMP] Parser failed\n");
        DESTROY_COMP_TOK_ARRAY(toks);
        AC_MFree(comp_ctx);
        return ASTRAC_ERR_AST;
    }

    /* ── Stage 4: Verify ────────────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 4/5: Verifying...\n");
    if (!COMP_VERIFY(ast, comp_ctx)) {
        AC_PRINTF("[COMP] Verification failed\n");
        DESTROY_CNODE_TREE(ast);
        DESTROY_COMP_TOK_ARRAY(toks);
        AC_MFree(comp_ctx);
        return ASTRAC_ERR_VERIFY;
    }

    /* ── Stage 5: Codegen ────────────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 5/5: Generating assembly...\n");

    /* Use a separate .AS output path so we don't clobber cfg->outfile */
    {
        U8 asm_path[512];
        AC_MEMZERO(asm_path, sizeof(asm_path));
        PU8 dot = AC_STRRCHR(cfg->outfile, '.');
        if (dot) {
            U32 base_len = (U32)(dot - cfg->outfile);
            AC_MEMCPY(asm_path, cfg->outfile, base_len);
        } else {
            AC_STRCPY(asm_path, cfg->outfile);
        }
        AC_STRCAT(asm_path, ".AS");
        comp_ctx->out_asm = AC_STRDUP(asm_path);
    }

    if (!COMP_GEN(ast, comp_ctx)) {
        AC_PRINTF("[COMP] Code generation failed\n");
        DESTROY_CNODE_TREE(ast);
        DESTROY_COMP_TOK_ARRAY(toks);
        AC_MFree(comp_ctx);
        return ASTRAC_ERR_CODEGEN;
    }

    /* Cleanup */
    DESTROY_CNODE_TREE(ast);
    DESTROY_COMP_TOK_ARRAY(toks);

    /* ── Assemble ────────────────────────────────────────────────────────── */
    if (cfg->stepoff_level != 2 && cfg->stepoff_level != 1 && cfg->build_type != BUILD_TYPE_PREPROCESS_ONLY) {
        cfg->input_file = comp_ctx->out_asm;
        if (cfg->verbose) AC_PRINTF("[COMP] Assembling %s...\n", comp_ctx->out_asm);
        ASTRAC_RESULT asm_res = START_ASSEMBLING();
        if (asm_res != ASTRAC_OK) { AC_MFree(comp_ctx); return asm_res; }
    }

    if (cfg->verbose) AC_PRINTF("[COMP] Compilation complete.\n");
    AC_MFree(comp_ctx);
    return ASTRAC_OK;
}
