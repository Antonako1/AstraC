/*
 * ASSEMBLER/AST.c — Assembler AST builder and token cursor (skeleton).
 *
 * Consumes the token stream from LEX() and produces an ASM_AST_ARRAY.
 * TODO: implement.
 */
#include "ASSEMBLER.h"
#include "MNEMONICS.h"

/* ── Token cursor helpers ─────────────────────────────────────────────────── */

PASM_TOK TOK_PEEK(TOK_CURSOR *cur) {
    /* TODO */
    (void)cur;
    return NULLPTR;
}

PASM_TOK TOK_ADVANCE(TOK_CURSOR *cur) {
    /* TODO */
    (void)cur;
    return NULLPTR;
}

BOOL TOK_AT_END(TOK_CURSOR *cur) {
    /* TODO */
    (void)cur;
    return TRUE;
}

BOOL TOK_MATCH(TOK_CURSOR *cur, ASM_TOKEN_TYPE type) {
    /* TODO */
    (void)cur; (void)type;
    return FALSE;
}

BOOL TOK_EXPECT(TOK_CURSOR *cur, ASM_TOKEN_TYPE type, PU8 ctx) {
    /* TODO */
    (void)cur; (void)type; (void)ctx;
    return FALSE;
}

VOID TOK_SKIP_EOL(TOK_CURSOR *cur) {
    /* TODO */
    (void)cur;
}

/* ── AST builder ──────────────────────────────────────────────────────────── */

ASM_AST_ARRAY *ASM_BUILD_AST(ASM_TOK_ARRAY *toks) {
    /* TODO */
    (void)toks;
    return NULLPTR;
}

VOID DESTROY_AST_ARR(ASM_AST_ARRAY *ast) {
    /* TODO */
    (void)ast;
}
