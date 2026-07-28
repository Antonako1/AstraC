/*
 * COMPILER/LEX.c — AC language lexer.
 *
 * Reads preprocessed AC source (ctx->tmp_src) and produces a COMP_TOK_ARRAY.
 * All identifiers and keywords are normalized to UPPER CASE.
 * Comment styles: semicolon (asm), double-slash (C line), slash-star (C block).
 */
#include "COMPILER.h"

STATIC VOID STR_UPPER(PU8 s) {
    while (*s) { if (*s >= 'a' && *s <= 'z') *s -= 32; s++; }
}

STATIC BOOL IS_ID_START(U8 c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

STATIC BOOL IS_ID_CONT(U8 c) {
    return IS_ID_START(c) || (c >= '0' && c <= '9');
}

STATIC const COMP_KW_MAP kw_map[] = {
    {CTOK_KW_U8, "U8"}, {CTOK_KW_I8, "I8"}, {CTOK_KW_U16, "U16"},
    {CTOK_KW_I16, "I16"}, {CTOK_KW_U32, "U32"}, {CTOK_KW_I32, "I32"},
    {CTOK_KW_PU8, "PU8"}, {CTOK_KW_PU16, "PU16"}, {CTOK_KW_PU32, "PU32"},
    {CTOK_KW_PPU8, "PPU8"}, {CTOK_KW_PPU16, "PPU16"}, {CTOK_KW_PPU32, "PPU32"},
    {CTOK_KW_PI8, "PI8"}, {CTOK_KW_PI16, "PI16"}, {CTOK_KW_PI32, "PI32"},
    {CTOK_KW_PPI8, "PPI8"}, {CTOK_KW_PPI16, "PPI16"}, {CTOK_KW_PPI32, "PPI32"},
    {CTOK_KW_F32, "F32"}, {CTOK_KW_U0, "U0"}, {CTOK_KW_BOOL, "BOOL"},
    {CTOK_KW_TRUE, "TRUE"}, {CTOK_KW_FALSE, "FALSE"}, {CTOK_KW_NULLPTR, "NULLPTR"},
    {CTOK_KW_VOIDPTR, "VOIDPTR"}, {CTOK_KW_VOID, "VOID"},
    {CTOK_KW_IF, "IF"}, {CTOK_KW_ELSE, "ELSE"}, {CTOK_KW_FOR, "FOR"},
    {CTOK_KW_WHILE, "WHILE"}, {CTOK_KW_DO, "DO"}, {CTOK_KW_RETURN, "RETURN"},
    {CTOK_KW_BREAK, "BREAK"}, {CTOK_KW_CONTINUE, "CONTINUE"},
    {CTOK_KW_SWITCH, "SWITCH"}, {CTOK_KW_CASE, "CASE"}, {CTOK_KW_DEFAULT, "DEFAULT"},
    {CTOK_KW_GOTO, "GOTO"}, {CTOK_KW_STRUCT, "STRUCT"}, {CTOK_KW_UNION, "UNION"},
    {CTOK_KW_ENUM, "ENUM"}, {CTOK_KW_SIZEOF, "SIZEOF"}, {CTOK_KW_ASM, "ASM"},
    {CTOK_KW_STATIC, "STATIC"}, {CTOK_KW_LOCAL, "LOCAL"},
    {CTOK_KW_TYPEDEF, "TYPEDEF"}, {CTOK_KW_VOID, "VOID"},
    {CTOK_EOF, NULLPTR}
};

STATIC COMP_TOK_TYPE COMP_KW_LOOKUP(PU8 s) {
    for (U32 i = 0; kw_map[i].kw_str; i++)
        if (AC_STRCMP(s, kw_map[i].kw_str) == 0) return kw_map[i].key;
    return CTOK_IDENT;
}

STATIC PCOMP_TOK TOK_NEW(COMP_TOK_TYPE type, PU8 txt, U32 line, U32 col) {
    PCOMP_TOK t = (PCOMP_TOK)AC_MAlloc(sizeof(COMP_TOK));
    if (!t) return NULLPTR;
    AC_MEMZERO(t, sizeof(COMP_TOK));
    t->type = type;
    t->line = line;
    t->col  = col;
    if (txt) { t->txt = AC_STRDUP(txt); }
    return t;
}

STATIC VOID TOK_ARR_APPEND(PCOMP_TOK_ARRAY arr, PCOMP_TOK tok) {
    if (arr->len >= arr->cap) {
        U32 ncap = arr->cap ? arr->cap * 2 : 256;
        PCOMP_TOK *n = (PCOMP_TOK *)AC_ReAlloc(arr->toks, ncap * sizeof(PCOMP_TOK));
        if (!n) return;
        arr->toks = n;
        arr->cap  = ncap;
    }
    arr->toks[arr->len++] = tok;
}

/* Read a hex digit value, return TRUE on success */
STATIC BOOL HEX_DIGIT(U8 c, U32 *v) {
    if (c >= '0' && c <= '9')      { *v = c - '0';      return TRUE; }
    if (c >= 'a' && c <= 'f')      { *v = c - 'a' + 10; return TRUE; }
    if (c >= 'A' && c <= 'F')      { *v = c - 'A' + 10; return TRUE; }
    return FALSE;
}

/* Read string escape sequence into *ch, advance pp past it */
STATIC VOID READ_ESCAPE(PU8 *pp, U8 *ch) {
    PU8 p = *pp;
    switch (*p) {
        case 'n': *ch = '\n'; p++; break;
        case 'r': *ch = '\r'; p++; break;
        case 't': *ch = '\t'; p++; break;
        case '0': *ch = '\0'; p++; break;
        case '\\':*ch = '\\'; p++; break;
        case '"': *ch = '"';  p++; break;
        case '\'':*ch = '\''; p++; break;
        default:  *ch = *p;   p++; break;
    }
    *pp = p;
}

PCOMP_TOK_ARRAY COMP_LEX(PCOMP_CTX ctx) {
    if (!ctx || !ctx->tmp_src) return NULLPTR;

    PCOMP_TOK_ARRAY arr = (PCOMP_TOK_ARRAY)AC_MAlloc(sizeof(COMP_TOK_ARRAY));
    if (!arr) return NULLPTR;
    AC_MEMZERO(arr, sizeof(COMP_TOK_ARRAY));

    PU8 src = ctx->tmp_src;
    U32 line = 1, col = 1;

    while (*src) {
        U8 c = (U8)*src;

        /* Skip whitespace */
        if (c == ' ' || c == '\t') { src++; col++; continue; }
        if (c == '\r') { src++; col = 1; continue; }
        if (c == '\n') { src++; line++; col = 1; continue; }

        /* Comments */
        if (c == '/' && src[1] == '/') {
            src += 2; while (*src && *src != '\n') src++; continue;
        }
        if (c == '/' && src[1] == '*') {
            U32 sl = line, sc = col;
            src += 2; col += 2;
            while (*src && !(*src == '*' && src[1] == '/')) {
                if (*src == '\n') { line++; col = 1; }
                else col++;
                src++;
            }
            if (!*src) {
                TOK_ARR_APPEND(arr, TOK_NEW(CTOK_ERROR, "unterminated comment", sl, sc));
                return arr;
            }
            src += 2; col += 2;
            continue;
        }

        /* Identifiers and keywords */
        if (IS_ID_START(c)) {
            U32 sl = line, sc = col;
            U8  buf[256]; U32 i = 0;
            buf[i++] = c; src++; col++;
            while (*src && IS_ID_CONT((U8)*src) && i < 254)
                { buf[i++] = (U8)*src; src++; col++; }
            buf[i] = '\0';
            STR_UPPER(buf);
            COMP_TOK_TYPE kw = COMP_KW_LOOKUP(buf);
            TOK_ARR_APPEND(arr, TOK_NEW(kw, buf, sl, sc));
            continue;
        }

        /* Numbers */
        if (c >= '0' && c <= '9') {
            U32 sl = line, sc = col;
            U32 val = 0;
            BOOL is_hex = FALSE, is_bin = FALSE, is_float = FALSE;

            if (c == '0' && (src[1] == 'x' || src[1] == 'X'))
                { is_hex = TRUE; src += 2; col += 2; }
            else if (c == '0' && (src[1] == 'b' || src[1] == 'B'))
                { is_bin = TRUE; src += 2; col += 2; }

            if (is_hex) {
                U32 d;
                while (*src && HEX_DIGIT((U8)*src, &d))
                    { val = (val << 4) | d; src++; col++; }
            } else if (is_bin) {
                while (*src && (*src == '0' || *src == '1'))
                    { val = (val << 1) | (U32)(*src - '0'); src++; col++; }
            } else {
                while (*src && *src >= '0' && *src <= '9')
                    { val = val * 10 + (U32)(*src - '0'); src++; col++; }
                if (*src == '.') {
                    is_float = TRUE;
                    src++; col++;
                    while (*src && *src >= '0' && *src <= '9')
                        { src++; col++; } /* skip fractional part */
                    /* Parse as float by converting integer part */
                }
            }

            if (is_float) {
                PCOMP_TOK t = TOK_NEW(CTOK_FLOAT_LIT, NULLPTR, sl, sc);
                t->fval = (F32)val;
                TOK_ARR_APPEND(arr, t);
            } else {
                PCOMP_TOK t = TOK_NEW(CTOK_INT_LIT, NULLPTR, sl, sc);
                t->ival = val;
                TOK_ARR_APPEND(arr, t);
            }
            continue;
        }

        /* String literal */
        if (c == '"') {
            U32 sl = line, sc = col;
            U8  buf[1024]; U32 i = 0;
            src++; col++;
            while (*src && *src != '"' && i < 1022) {
                if (*src == '\\') { src++; col++; U8 ec; READ_ESCAPE(&src, &ec); buf[i++] = ec; }
                else { buf[i++] = (U8)*src; src++; col++; }
            }
            if (*src == '"') { src++; col++; }
            buf[i] = '\0';
            TOK_ARR_APPEND(arr, TOK_NEW(CTOK_STR_LIT, buf, sl, sc));
            continue;
        }

        /* Character literal */
        if (c == '\'') {
            U32 sl = line, sc = col;
            U8  ch = 0;
            src++; col++;
            if (*src == '\\') { src++; col++; READ_ESCAPE(&src, &ch); }
            else { ch = (U8)*src; src++; col++; }
            if (*src == '\'') { src++; col++; }
            PCOMP_TOK t = TOK_NEW(CTOK_CHAR_LIT, NULLPTR, sl, sc);
            t->ival = ch;
            TOK_ARR_APPEND(arr, t);
            continue;
        }

        U32 sl = line, sc = col;

        /* Two-character operators */
        if ((c == '+' && src[1] == '+') || (c == '-' && src[1] == '-')) {
            COMP_TOK_TYPE tt = (c == '+') ? CTOK_PLUSPLUS : CTOK_MINUSMINUS;
            src += 2; col += 2;
            TOK_ARR_APPEND(arr, TOK_NEW(tt, NULLPTR, sl, sc));
            continue;
        }
        if (c == '<' && src[1] == '<') {
            if (src[2] == '=') { src += 3; col += 3; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_SHL_ASSIGN, NULLPTR, sl, sc)); }
            else { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_SHL, NULLPTR, sl, sc)); }
            continue;
        }
        if (c == '>' && src[1] == '>') {
            if (src[2] == '=') { src += 3; col += 3; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_SHR_ASSIGN, NULLPTR, sl, sc)); }
            else { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_SHR, NULLPTR, sl, sc)); }
            continue;
        }
        if (c == '<' && src[1] == '=') { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_LE, NULLPTR, sl, sc)); continue; }
        if (c == '>' && src[1] == '=') { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_GE, NULLPTR, sl, sc)); continue; }
        if (c == '=' && src[1] == '=') { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_EQ, NULLPTR, sl, sc)); continue; }
        if (c == '!' && src[1] == '=') { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_NEQ, NULLPTR, sl, sc)); continue; }
        if (c == '&' && src[1] == '&') { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_AND, NULLPTR, sl, sc)); continue; }
        if (c == '|' && src[1] == '|') { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_OR, NULLPTR, sl, sc)); continue; }
        if (c == '-' && src[1] == '>') { src += 2; col += 2; TOK_ARR_APPEND(arr, TOK_NEW(CTOK_ARROW, NULLPTR, sl, sc)); continue; }

        /* Compound assignment (+=, -=, etc.) */
        if (src[1] == '=') {
            COMP_TOK_TYPE tt;
            switch (c) {
                case '+': tt = CTOK_PLUS_ASSIGN;  break;
                case '-': tt = CTOK_MINUS_ASSIGN; break;
                case '*': tt = CTOK_STAR_ASSIGN;  break;
                case '/': tt = CTOK_SLASH_ASSIGN; break;
                case '%': tt = CTOK_PCT_ASSIGN;   break;
                case '&': tt = CTOK_AMP_ASSIGN;   break;
                case '|': tt = CTOK_PIPE_ASSIGN;  break;
                case '^': tt = CTOK_CARET_ASSIGN; break;
                default:  goto single_char;
            }
            src += 2; col += 2;
            TOK_ARR_APPEND(arr, TOK_NEW(tt, NULLPTR, sl, sc));
            continue;
        }

single_char:
        /* Single character tokens */
        COMP_TOK_TYPE st;
        switch (c) {
            case '+':  st = CTOK_PLUS;     break;
            case '-':  st = CTOK_MINUS;    break;
            case '*':  st = CTOK_STAR;     break;
            case '/':  st = CTOK_SLASH;    break;
            case '%':  st = CTOK_PERCENT;  break;
            case '=':  st = CTOK_ASSIGN;   break;
            case '<':  st = CTOK_LT;       break;
            case '>':  st = CTOK_GT;       break;
            case '!':  st = CTOK_NOT;      break;
            case '&':  st = CTOK_AMP;      break;
            case '|':  st = CTOK_PIPE;     break;
            case '^':  st = CTOK_CARET;    break;
            case '~':  st = CTOK_TILDE;    break;
            case '(':  st = CTOK_LPAREN;   break;
            case ')':  st = CTOK_RPAREN;   break;
            case '{':  st = CTOK_LBRACE;   break;
            case '}':  st = CTOK_RBRACE;   break;
            case '[':  st = CTOK_LBRACKET; break;
            case ']':  st = CTOK_RBRACKET; break;
            case ';':  st = CTOK_SEMICOLON; break;
            case ',':  st = CTOK_COMMA;    break;
            case '?':  st = CTOK_QUESTION; break;
            case ':':  st = CTOK_COLON;    break;
            case '.':  st = CTOK_DOT;      break;
            default:
                TOK_ARR_APPEND(arr, TOK_NEW(CTOK_ERROR, "unexpected character", sl, sc));
                src++; col++;
                continue;
        }
        src++; col++;
        TOK_ARR_APPEND(arr, TOK_NEW(st, NULLPTR, sl, sc));
    }

    TOK_ARR_APPEND(arr, TOK_NEW(CTOK_EOF, NULLPTR, line, col));
    return arr;
}

VOID DESTROY_COMP_TOK_ARRAY(PCOMP_TOK_ARRAY toks) {
    if (!toks) return;
    for (U32 i = 0; i < toks->len; i++) {
        if (toks->toks[i] && toks->toks[i]->txt)
            AC_MFree(toks->toks[i]->txt);
        AC_MFree(toks->toks[i]);
    }
    if (toks->toks) AC_MFree(toks->toks);
    AC_MFree(toks);
}
