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

STATIC VOID FREE_COMP_CTX(COMP_CTX *comp_ctx, PU8 src_copy) {
    if (!comp_ctx) return;
    SYM_FREE_ALL();
    if (comp_ctx->rodata_strings) AC_MFree(comp_ctx->rodata_strings);
    if (comp_ctx->imports)        AC_MFree(comp_ctx->imports);
    if (comp_ctx->files) {
        for (U32 i = 0; i < comp_ctx->file_count; i++) {
            if (comp_ctx->files[i]) AC_MFree(comp_ctx->files[i]);
        }
        AC_MFree(comp_ctx->files);
    }
    if (src_copy)            AC_MFree(src_copy);
    if (comp_ctx->src_lines) AC_MFree(comp_ctx->src_lines);
    if (comp_ctx->tmp_src)   AC_MFree(comp_ctx->tmp_src);
    if (comp_ctx->out_asm)   AC_MFree(comp_ctx->out_asm);
    AC_MFree(comp_ctx);
}

U32 START_COMPILER() {
    ASTRAC_ARGS *cfg = GET_ARGS();
    if (!cfg || !cfg->input_file) {
        AC_PRINTF_ERR("[COMP] Error: no input file\n");
        return ASTRAC_ERR_INTERNAL;
    }

    COMP_CTX *comp_ctx = (COMP_CTX *)AC_MAlloc(sizeof(COMP_CTX));
    if (!comp_ctx) { AC_PRINTF("[COMP] Out of memory\n"); return ASTRAC_ERR_INTERNAL; }
    AC_MEMZERO(comp_ctx, sizeof(COMP_CTX));
    comp_ctx->verbose    = cfg->verbose;
    comp_ctx->debug      = cfg->debug;
    comp_ctx->file_scope = 1;
    PU8 src_copy         = NULLPTR;

    /* ── Stage 1: Preprocessing ────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 1/5: Preprocessing...\n");

    PASM_INFO info = PREPROCESS_C();
    if (!info || info->tmp_file_count == 0 || !info->tmp_files[0]) {
        AC_PRINTF_ERR("[COMP] Preprocessor failed\n");
        if (info) DESTROY_ASM_INFO(info);
        FREE_COMP_CTX(comp_ctx, src_copy);
        return ASTRAC_ERR_PREPROCESS;
    }
    PU8 tmp_src_path = info->tmp_files[0];

    if (cfg->stepoff_level == 1 || cfg->build_type == BUILD_TYPE_PREPROCESS_ONLY) {
        AC_PRINTF("[COMP] Preprocess-only: output in %s\n", tmp_src_path);
        DESTROY_ASM_INFO(info);
        FREE_COMP_CTX(comp_ctx, src_copy);
        return ASTRAC_OK;
    }

    comp_ctx->tmp_src = NULLPTR;

    /* Read preprocessed source into memory */
    {
        FILE *pf = fopen((const char *)tmp_src_path, "rb");
        if (!pf) {
            AC_PRINTF("[COMP] Cannot open preprocessed file: %s\n", tmp_src_path);
            DESTROY_ASM_INFO(info);
            FREE_COMP_CTX(comp_ctx, src_copy);
            return ASTRAC_ERR_PREPROCESS;
        }
        fseek(pf, 0, SEEK_END);
        U32 psize = (U32)ftell(pf);
        fseek(pf, 0, SEEK_SET);
        comp_ctx->tmp_src = (PU8)AC_MAlloc(psize + 1);
        if (!comp_ctx->tmp_src) {
            fclose(pf);
            DESTROY_ASM_INFO(info);
            FREE_COMP_CTX(comp_ctx, src_copy);
            return ASTRAC_ERR_INTERNAL;
        }
        fread(comp_ctx->tmp_src, 1, psize, pf);
        comp_ctx->tmp_src[psize] = '\0';
        fclose(pf);
    }

    /* Remove temporary .I file unless debug mode is enabled */
    if (!cfg->debug && AC_FILE_EXISTS(tmp_src_path)) {
        AC_FILE_DELETE(tmp_src_path);
    }
    DESTROY_ASM_INFO(info);

    /* Build source-line index for debug mode (aware of #line markers) */
    if (comp_ctx->debug) {
        PU8 src = comp_ctx->tmp_src;
        U32 len = AC_STRLEN(src);
        U32 max_line = 0, cur_line = 1;
        PU8 p = src;
        while (p < src + len) {
            PU8 ls = p;
            while (p < src + len && *p != '\n') p++;
            if (p < src + len) p++;
            if (ls[0] == '#' && ls[1] == 'l' && ls[2] == 'i' && ls[3] == 'n' && ls[4] == 'e'
                && (ls[5] == ' ' || ls[5] == '\t')) {
                PU8 lp = ls + 5;
                while (*lp == ' ' || *lp == '\t') lp++;
                U32 lnum = 0;
                while (*lp >= '0' && *lp <= '9') { lnum = lnum * 10 + (U32)(*lp - '0'); lp++; }
                if (lnum > 0) cur_line = lnum;
            } else {
                if (cur_line > max_line) max_line = cur_line;
                cur_line++;
            }
        }
        comp_ctx->src_line_count = max_line;
        if (max_line > 0) {
            comp_ctx->src_lines = (PU8 *)AC_MAlloc(sizeof(PU8) * max_line);
            if (comp_ctx->src_lines) {
                AC_MEMZERO(comp_ctx->src_lines, sizeof(PU8) * max_line);
                src_copy = (PU8)AC_MAlloc(len + 1);
                if (src_copy) {
                    AC_MEMCPY(src_copy, src, len + 1);
                    cur_line = 1;
                    p = src_copy;
                    while (p < src_copy + len) {
                        PU8 ls = p;
                        while (p < src_copy + len && *p != '\n') p++;
                        if (p > ls && *(p - 1) == '\r') *(p - 1) = '\0';
                        if (p < src_copy + len) *p++ = '\0';
                        if (ls[0] == '#' && ls[1] == 'l' && ls[2] == 'i' && ls[3] == 'n' && ls[4] == 'e'
                            && (ls[5] == ' ' || ls[5] == '\t')) {
                            PU8 lp = ls + 5;
                            while (*lp == ' ' || *lp == '\t') lp++;
                            U32 lnum = 0;
                            while (*lp >= '0' && *lp <= '9') { lnum = lnum * 10 + (U32)(*lp - '0'); lp++; }
                            if (lnum > 0) cur_line = lnum;
                        } else {
                            if (cur_line >= 1 && cur_line <= max_line && !comp_ctx->src_lines[cur_line - 1])
                                comp_ctx->src_lines[cur_line - 1] = ls;
                            cur_line++;
                        }
                    }
                }
            }
        }
    }

    /* ── Stage 2: Lex ──────────────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 2/5: Lexing...\n");
    PCOMP_TOK_ARRAY toks = COMP_LEX(comp_ctx);
    if (!toks) {
        AC_PRINTF_ERR("[COMP] Lexer failed\n");
        FREE_COMP_CTX(comp_ctx, src_copy);
        return ASTRAC_ERR_LEX;
    }

    /* ── Stage 3: Parse ─────────────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 3/5: Parsing...\n");
    PCNODE ast = COMP_PARSE(toks, comp_ctx);
    if (!ast) {
        AC_PRINTF_ERR("[COMP] Parser failed\n");
        DESTROY_COMP_TOK_ARRAY(toks);
        FREE_COMP_CTX(comp_ctx, src_copy);
        return ASTRAC_ERR_AST;
    }

    /* ── Stage 4: Verify ────────────────────────────────────────────────── */
    if (cfg->verbose) AC_PRINTF("[COMP] Stage 4/5: Verifying...\n");
    if (!COMP_VERIFY(ast, comp_ctx)) {
        AC_PRINTF_ERR("[COMP] Verification failed\n");
        DESTROY_CNODE_TREE(ast);
        DESTROY_COMP_TOK_ARRAY(toks);
        FREE_COMP_CTX(comp_ctx, src_copy);
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
        AC_PRINTF_ERR("[COMP] Code generation failed\n");
        DESTROY_CNODE_TREE(ast);
        DESTROY_COMP_TOK_ARRAY(toks);
        FREE_COMP_CTX(comp_ctx, src_copy);
        return ASTRAC_ERR_CODEGEN;
    }

    DESTROY_CNODE_TREE(ast);
    DESTROY_COMP_TOK_ARRAY(toks);

    /* ── Assemble ────────────────────────────────────────────────────────── */
    if (cfg->stepoff_level != 2 && cfg->stepoff_level != 1 && cfg->build_type != BUILD_TYPE_PREPROCESS_ONLY) {
        PU8 saved_out_asm = AC_STRDUP(comp_ctx->out_asm);
        cfg->input_file = saved_out_asm;
        if (cfg->verbose) AC_PRINTF("[COMP] Assembling %s...\n", saved_out_asm);
        ASTRAC_RESULT asm_res = START_ASSEMBLING();
        if (saved_out_asm) AC_MFree(saved_out_asm);
        FREE_COMP_CTX(comp_ctx, src_copy);
        if (asm_res != ASTRAC_OK) return asm_res;
        if (cfg->verbose) AC_PRINTF("[COMP] Compilation complete.\n");
        return ASTRAC_OK;
    }

    if (cfg->verbose) AC_PRINTF("[COMP] Compilation complete.\n");
    FREE_COMP_CTX(comp_ctx, src_copy);
    return ASTRAC_OK;
}
