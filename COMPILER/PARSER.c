/*
 * COMPILER/PARSER.c — AC language recursive-descent parser.
 *
 * Consumes a COMP_TOK_ARRAY and returns a PCNODE tree rooted at CNODE_PROGRAM.
 * Builds the symbol table in ctx->symtab during parsing.
 */
#include "COMPILER.h"

/* ── Global parser state ──────────────────────────────────────────────────── */
STATIC PCOMP_TOK_ARRAY toks;
STATIC U32            pos;
STATIC PCOMP_CTX      ctx;
STATIC SYM_TABLE      *sym;

/* ── Helpers ──────────────────────────────────────────────────────────────── */
STATIC PCOMP_TOK PEEK()  { return (pos < toks->len) ? toks->toks[pos] : NULLPTR; }
STATIC PCOMP_TOK ADV()   { return (pos < toks->len) ? toks->toks[pos++] : NULLPTR; }
STATIC BOOL     MATCH(COMP_TOK_TYPE t) { return (PEEK() && PEEK()->type == t); }
STATIC PCOMP_TOK EXPECT(COMP_TOK_TYPE t) {
    if (MATCH(t)) return ADV();
    if (PEEK()) AC_PRINTF("[PARSE] L%u:%u expected token type %u, got %u\n",
        PEEK()->line, PEEK()->col, t, PEEK()->type);
    return NULLPTR;
}

STATIC VOID SKIP_TO_SEMI() {
    while (PEEK() && PEEK()->type != CTOK_SEMICOLON && PEEK()->type != CTOK_RBRACE
           && PEEK()->type != CTOK_EOF) ADV();
}

STATIC SYMBOL *SYM_ADD(PU8 name, SYM_KIND kind) {
    for (U32 i = 0; i < sym->count; i++)
        if (sym->entries[i].name && AC_STRCMP(sym->entries[i].name, name) == 0) {
            /* File-local symbols from different scopes don't collide */
            if (sym->entries[i].is_file_local && sym->entries[i].file_scope != ctx->file_scope)
                continue;
            return &sym->entries[i];
        }
    if (sym->count >= 512) return NULLPTR;
    SYMBOL *s = &sym->entries[sym->count++];
    AC_MEMZERO(s, sizeof(SYMBOL));
    s->name = AC_STRDUP(name);
    s->kind = kind;
    return s;
}

STATIC SYMBOL *SYM_LOOKUP(PU8 name) {
    for (U32 i = 0; i < sym->count; i++)
        if (sym->entries[i].name && AC_STRCMP(sym->entries[i].name, name) == 0) {
            if (sym->entries[i].is_file_local && sym->entries[i].file_scope != ctx->file_scope)
                return NULLPTR; /* file-local from another file — invisible */
            return &sym->entries[i];
        }
    return NULLPTR;
}

/* TRUE if a symbol names a usable type (typedef, struct, union, or enum). */
STATIC BOOL SYM_IS_TYPE(SYMBOL *s) {
    return s && (s->kind == SYM_TYPEDEF || s->kind == SYM_STRUCT
              || s->kind == SYM_UNION  || s->kind == SYM_ENUM);
}

STATIC VOID SYM_FREE_ALL() {
    for (U32 i = 0; i < sym->count; i++) {
        if (sym->entries[i].name) AC_MFree(sym->entries[i].name);
        for (U32 j = 0; j < sym->entries[i].param_count; j++)
            if (sym->entries[i].param_names[j]) AC_MFree(sym->entries[i].param_names[j]);
        for (U32 j = 0; j < sym->entries[i].field_count; j++)
            if (sym->entries[i].fields[j].name) AC_MFree(sym->entries[i].fields[j].name);
    }
}

STATIC COMP_TYPE COMP_MAKE_TYPE(COMP_BASE_TYPE base, U8 ptr_depth, PU8 name) {
    COMP_TYPE t;
    t.base = base; t.ptr_depth = ptr_depth; t.name = name;
    return t;
}

/* Size of a type as used in a struct/union field.  Nested struct/union types
 * are resolved through the symbol table (COMP_TYPE_SIZE alone returns 0 for
 * struct/union types). */
STATIC U32 FIELD_TYPE_SIZE(COMP_TYPE t) {
    if (t.base == CTYPE_STRUCT || t.base == CTYPE_UNION) {
        if (t.name) {
            SYMBOL *s = SYM_LOOKUP(t.name);
            if (s) return s->total_size;
        }
        return 0;
    }
    return COMP_TYPE_SIZE(t);
}

/* ── Forward declarations ─────────────────────────────────────────────────── */
STATIC PCNODE parse_expr();
STATIC PCNODE parse_expr_prec(U32 min_prec);
STATIC PCNODE parse_stmt();
STATIC PCNODE parse_block();
STATIC COMP_TYPE parse_type();
STATIC PCNODE parse_atom();
STATIC PCNODE parse_postfix(PCNODE lhs);

/* ── Type parsing ─────────────────────────────────────────────────────────── */
STATIC COMP_TYPE parse_type() {
    /* Pointer depth: first check for PU8, PPU32 etc. as keyword types */
    COMP_BASE_TYPE bt = CTYPE_NONE;
    U8 pd = 0;
    PU8 name = NULLPTR;

    if (MATCH(CTOK_KW_U8))  { ADV(); bt = CTYPE_U8; }
    else if (MATCH(CTOK_KW_U16))   { ADV(); bt = CTYPE_U16; }
    else if (MATCH(CTOK_KW_U32))   { ADV(); bt = CTYPE_U32; }
    else if (MATCH(CTOK_KW_I8))    { ADV(); bt = CTYPE_I8; }
    else if (MATCH(CTOK_KW_I16))   { ADV(); bt = CTYPE_I16; }
    else if (MATCH(CTOK_KW_I32))   { ADV(); bt = CTYPE_I32; }
    else if (MATCH(CTOK_KW_F32))   { ADV(); bt = CTYPE_F32; }
    else if (MATCH(CTOK_KW_BOOL))  { ADV(); bt = CTYPE_BOOL; }
    else if (MATCH(CTOK_KW_U0) || MATCH(CTOK_KW_VOID)) { ADV(); bt = CTYPE_U0; }
    else if (MATCH(CTOK_KW_VOIDPTR)) { ADV(); bt = CTYPE_VOIDPTR; }
    else if (MATCH(CTOK_KW_PU8))     { ADV(); bt = CTYPE_PU8;  }
    else if (MATCH(CTOK_KW_PU16))    { ADV(); bt = CTYPE_PU16; }
    else if (MATCH(CTOK_KW_PU32))    { ADV(); bt = CTYPE_PU32; }
    else if (MATCH(CTOK_KW_PPU8))    { ADV(); bt = CTYPE_PPU8; }
    else if (MATCH(CTOK_KW_PPU16))   { ADV(); bt = CTYPE_PPU16; }
    else if (MATCH(CTOK_KW_PPU32))   { ADV(); bt = CTYPE_PPU32; }
    else if (MATCH(CTOK_KW_PI8))     { ADV(); bt = CTYPE_PI8;  }
    else if (MATCH(CTOK_KW_PI16))    { ADV(); bt = CTYPE_PI16; }
    else if (MATCH(CTOK_KW_PI32))    { ADV(); bt = CTYPE_PI32; }
    else if (MATCH(CTOK_KW_PPI8))    { ADV(); bt = CTYPE_PPI8; }
    else if (MATCH(CTOK_KW_PPI16))   { ADV(); bt = CTYPE_PPI16; }
    else if (MATCH(CTOK_KW_PPI32))   { ADV(); bt = CTYPE_PPI32; }
    else if (MATCH(CTOK_KW_STRUCT) || MATCH(CTOK_KW_UNION) || MATCH(CTOK_KW_ENUM)) {
        COMP_TOK_TYPE bt_tok = PEEK()->type;
        ADV();
        if (MATCH(CTOK_IDENT)) {
            PCOMP_TOK n = ADV();
            name = n->txt;
            SYMBOL *s = SYM_LOOKUP(name);
            if (!s) { AC_PRINTF("[PARSE] L%u unknown type '%s'\n", n->line, name); }
        }
        bt = (bt_tok == CTOK_KW_STRUCT) ? CTYPE_STRUCT :
             (bt_tok == CTOK_KW_UNION)  ? CTYPE_UNION  : CTYPE_ENUM;
    } else if (MATCH(CTOK_IDENT)) {
        /* Typedef name or bare struct/union/enum tag name */
        PCOMP_TOK n = ADV();
        SYMBOL *s = SYM_LOOKUP(n->txt);
        if (s && s->kind == SYM_TYPEDEF) {
            bt   = s->type.base;
            pd   = s->type.ptr_depth;
            name = s->type.name;
        } else if (s && s->kind == SYM_STRUCT) {
            bt = CTYPE_STRUCT; pd = 0; name = n->txt;
        } else if (s && s->kind == SYM_UNION) {
            bt = CTYPE_UNION;  pd = 0; name = n->txt;
        } else if (s && s->kind == SYM_ENUM) {
            bt = CTYPE_ENUM;   pd = 0; name = n->txt;
        } else {
            AC_PRINTF("[PARSE] L%u unknown type '%s'\n", n->line, n->txt);
        }
    }

    /* Handle trailing * for extra pointer depth */
    while (MATCH(CTOK_STAR)) { ADV(); pd++; }
    return COMP_MAKE_TYPE(bt, pd, name);
}

/* ── Expression parsing (Pratt/TDOP style) ────────────────────────────────── */

STATIC U32 PREC_RANGE(U32 lo, U32 hi) {
    return lo + (hi << 8);
}

STATIC U32 GET_PREC(COMP_TOK_TYPE op) {
    switch (op) {
        case CTOK_COMMA:        return PREC_RANGE(1,2);
        case CTOK_ASSIGN: case CTOK_PLUS_ASSIGN:
        case CTOK_MINUS_ASSIGN: case CTOK_STAR_ASSIGN:
        case CTOK_SLASH_ASSIGN: case CTOK_PCT_ASSIGN:
        case CTOK_AMP_ASSIGN: case CTOK_PIPE_ASSIGN:
        case CTOK_CARET_ASSIGN: case CTOK_SHL_ASSIGN:
        case CTOK_SHR_ASSIGN:   return PREC_RANGE(2,3);
        case CTOK_QUESTION:     return PREC_RANGE(3,4);
        case CTOK_OR:           return PREC_RANGE(4,5);
        case CTOK_AND:          return PREC_RANGE(5,6);
        case CTOK_PIPE:         return PREC_RANGE(6,7);
        case CTOK_CARET:        return PREC_RANGE(7,8);
        case CTOK_AMP:          return PREC_RANGE(8,9);
        case CTOK_EQ: case CTOK_NEQ: return PREC_RANGE(9,10);
        case CTOK_LT: case CTOK_GT:
        case CTOK_LE: case CTOK_GE:  return PREC_RANGE(10,11);
        case CTOK_SHL: case CTOK_SHR: return PREC_RANGE(11,12);
        case CTOK_PLUS: case CTOK_MINUS: return PREC_RANGE(12,13);
        case CTOK_STAR: case CTOK_SLASH: case CTOK_PERCENT: return PREC_RANGE(13,14);
        default: return PREC_RANGE(0,0);
    }
}

STATIC BOOL IS_UNARY(COMP_TOK_TYPE op) {
    return op == CTOK_MINUS || op == CTOK_PLUS || op == CTOK_NOT
        || op == CTOK_TILDE || op == CTOK_STAR || op == CTOK_AMP
        || op == CTOK_PLUSPLUS || op == CTOK_MINUSMINUS
        || op == CTOK_KW_SIZEOF;
}

/* Parse an expression atom */
STATIC PCNODE parse_atom() {
    PCOMP_TOK t = PEEK();
    if (!t) return NULLPTR;

    /* Parenthesized */
    if (MATCH(CTOK_LPAREN)) {
        ADV();
        /* Cast: (type)expr — only attempt if next is a known type keyword
         * or a typedef name, to avoid misleading "unknown type" diagnostics
         * on ordinary parenthesized expressions like (a + b). */
        U32 save = pos;
        COMP_TOK_TYPE next = PEEK()->type;
        BOOL try_cast = (next >= CTOK_KW_U8 && next <= CTOK_KW_PPI32)
                     || next == CTOK_KW_F32 || next == CTOK_KW_BOOL
                     || next == CTOK_KW_U0 || next == CTOK_KW_VOIDPTR
                     || next == CTOK_KW_VOID || next == CTOK_KW_STRUCT
                     || next == CTOK_KW_UNION || next == CTOK_KW_ENUM;
        if (next == CTOK_IDENT) {
            SYMBOL *ts = SYM_LOOKUP(PEEK()->txt);
            try_cast = SYM_IS_TYPE(ts);
        }
        if (try_cast) {
            COMP_TYPE ct = parse_type();
            if (ct.base != CTYPE_NONE && MATCH(CTOK_RPAREN)) {
                ADV();
                PCNODE e = parse_expr_prec(2);
                PCNODE n = CNODE_NEW(CNODE_CAST, t->line, t->col);
                n->dtype = ct;
                if (e) CNODE_ADD_CHILD(n, e);
                return n;
            }
            pos = save; /* backtrack: not a cast */
        }
        PCNODE e = parse_expr();
        EXPECT(CTOK_RPAREN);
        return e;
    }

    /* Unary operators */
    if (IS_UNARY(t->type)) {
        COMP_TOK_TYPE op = t->type; ADV();
        PCNODE n;
        BOOL is_pre = (op == CTOK_PLUSPLUS || op == CTOK_MINUSMINUS);
        if (op == CTOK_KW_SIZEOF) {
            if (MATCH(CTOK_LPAREN)) {
                U32 s2 = pos; ADV();
                COMP_TYPE st = parse_type();
                if (st.base != CTYPE_NONE && MATCH(CTOK_RPAREN)) { ADV(); n = CNODE_NEW(CNODE_SIZEOF_TYPE, t->line, t->col); n->dtype = st; return n; }
                pos = s2; PCNODE se = parse_expr(); EXPECT(CTOK_RPAREN);
                n = CNODE_NEW(CNODE_SIZEOF_EXPR, t->line, t->col); if (se) CNODE_ADD_CHILD(n, se); return n;
            }
            PCNODE se = parse_atom();
            n = CNODE_NEW(CNODE_SIZEOF_EXPR, t->line, t->col);
            if (se) CNODE_ADD_CHILD(n, se);
            return n;
        }
        if (op == CTOK_AMP) n = CNODE_NEW(CNODE_ADDR, t->line, t->col);
        else if (op == CTOK_STAR) n = CNODE_NEW(CNODE_DEREF, t->line, t->col);
        else if (is_pre) n = CNODE_NEW(CNODE_UNARY, t->line, t->col);
        else n = CNODE_NEW(CNODE_UNARY, t->line, t->col);
        n->op = op;
        /* The operand is a prefix expression: postfix binds tighter than
         * prefix unary, so `!f()` parses as `!(f())`, `*p->x` as `*(p->x)`. */
        PCNODE a = parse_atom();
        if (a) {
            a = parse_postfix(a);
            CNODE_ADD_CHILD(n, a);
        }
        return n;
    }

    /* Literals */
    if (MATCH(CTOK_INT_LIT)) {
        ADV();
        return CNODE_INT(t->ival, t->line, t->col);
    }
    if (MATCH(CTOK_FLOAT_LIT)) {
        ADV();
        return CNODE_FLOAT(t->fval, t->line, t->col);
    }
    if (MATCH(CTOK_STR_LIT)) {
        ADV();
        PCNODE n = CNODE_STR(t->txt, t->line, t->col);
        /* Assign rodata label */
        if (ctx->rodata_string_count < 256) {
            ctx->rodata_strings[ctx->rodata_string_count].label = ++ctx->label_counter;
            ctx->rodata_strings[ctx->rodata_string_count].node  = n;
            n->ival = ctx->rodata_strings[ctx->rodata_string_count].label;
            ctx->rodata_string_count++;
        }
        return n;
    }
    if (MATCH(CTOK_CHAR_LIT)) {
        ADV();
        return CNODE_INT(t->ival, t->line, t->col);
    }
    if (MATCH(CTOK_KW_TRUE))  { ADV(); PCNODE n = CNODE_NEW(CNODE_TRUE_LIT, t->line, t->col);  n->ival = 1; return n; }
    if (MATCH(CTOK_KW_FALSE)) { ADV(); PCNODE n = CNODE_NEW(CNODE_FALSE_LIT, t->line, t->col); n->ival = 0; return n; }
    if (MATCH(CTOK_KW_NULLPTR)) { ADV(); return CNODE_NEW(CNODE_NULLPTR, t->line, t->col); }

    /* Identifier */
    if (MATCH(CTOK_IDENT)) {
        ADV();
        PCNODE n = CNODE_IDENT_NODE(t->txt, t->line, t->col);
        return n;
    }

    AC_PRINTF("[PARSE] L%u unexpected token type %u\n", t->line, t->type);
    ADV(); return CNODE_INT(0, t->line, t->col);
}

/* Parse postfix operations (call, index, member, ++, --) */
STATIC PCNODE parse_postfix(PCNODE lhs) {
    while (PEEK()) {
        COMP_TOK_TYPE tt = PEEK()->type;
        if (tt == CTOK_LPAREN) {
            /* Function call */
            ADV();
            PCNODE n = CNODE_NEW(CNODE_CALL, lhs->line, lhs->col);
            n->txt = lhs->txt ? AC_STRDUP(lhs->txt) : NULLPTR;
            if (!MATCH(CTOK_RPAREN)) {
                do { CNODE_ADD_CHILD(n, parse_expr_prec(2)); } while (MATCH(CTOK_COMMA) && ADV());
            }
            EXPECT(CTOK_RPAREN);
            lhs = n;
        } else if (tt == CTOK_LBRACKET) {
            /* Array/index */
            ADV();
            PCNODE n = CNODE_NEW(CNODE_INDEX, lhs->line, lhs->col);
            CNODE_ADD_CHILD(n, lhs);
            CNODE_ADD_CHILD(n, parse_expr());
            EXPECT(CTOK_RBRACKET);
            lhs = n;
        } else if (tt == CTOK_DOT || tt == CTOK_ARROW) {
            ADV();
            COMP_TOK_TYPE ot = tt;
            PCOMP_TOK field = EXPECT(CTOK_IDENT);
            PCNODE n = CNODE_NEW((ot == CTOK_DOT) ? CNODE_MEMBER : CNODE_ARROW_EXPR, lhs->line, lhs->col);
            CNODE_ADD_CHILD(n, lhs);
            if (field) {
                PCNODE fn = CNODE_IDENT_NODE(field->txt, field->line, field->col);
                CNODE_ADD_CHILD(n, fn);
            }
            lhs = n;
        } else if (tt == CTOK_PLUSPLUS || tt == CTOK_MINUSMINUS) {
            ADV();
            PCNODE n = CNODE_NEW(CNODE_POSTFIX, lhs->line, lhs->col);
            n->op = tt;
            CNODE_ADD_CHILD(n, lhs);
            lhs = n;
        } else {
            break;
        }
    }
    return lhs;
}

STATIC PCNODE parse_expr_prec(U32 min_prec) {
    PCNODE lhs = parse_atom();
    if (!lhs) return NULLPTR;
    lhs = parse_postfix(lhs);

    while (PEEK()) {
        COMP_TOK_TYPE op = PEEK()->type;
        U32 prec = GET_PREC(op);
        U32 lop = prec & 0xFF, hip = prec >> 8;
        if (lop < (min_prec & 0xFF)) break;
        if (op == CTOK_ASSIGN || op == CTOK_PLUS_ASSIGN || op == CTOK_MINUS_ASSIGN
            || op == CTOK_STAR_ASSIGN || op == CTOK_SLASH_ASSIGN || op == CTOK_PCT_ASSIGN
            || op == CTOK_AMP_ASSIGN || op == CTOK_PIPE_ASSIGN || op == CTOK_CARET_ASSIGN
            || op == CTOK_SHL_ASSIGN || op == CTOK_SHR_ASSIGN) {
            ADV();
            PCNODE n = CNODE_NEW(CNODE_ASSIGN, lhs->line, lhs->col);
            n->op = op;
            CNODE_ADD_CHILD(n, lhs);
            CNODE_ADD_CHILD(n, parse_expr_prec(lop));
            lhs = n;
        } else if (op == CTOK_QUESTION) {
            ADV();
            PCNODE mid = parse_expr();
            EXPECT(CTOK_COLON);
            PCNODE right = parse_expr();
            PCNODE n = CNODE_NEW(CNODE_TERNARY, lhs->line, lhs->col);
            CNODE_ADD_CHILD(n, lhs);
            if (mid) CNODE_ADD_CHILD(n, mid);
            if (right) CNODE_ADD_CHILD(n, right);
            lhs = n;
        } else if (op == CTOK_COMMA) {
            ADV();
            PCNODE rhs = parse_expr();
            PCNODE n = CNODE_NEW(CNODE_BINARY, lhs->line, lhs->col);
            n->op = CTOK_COMMA;
            CNODE_ADD_CHILD(n, lhs);
            if (rhs) CNODE_ADD_CHILD(n, rhs);
            lhs = n;
        } else if (lop > 0) {
            ADV();
            PCNODE rhs = parse_expr_prec(hip);
            PCNODE n = CNODE_NEW(CNODE_BINARY, lhs->line, lhs->col);
            n->op = op;
            CNODE_ADD_CHILD(n, lhs);
            if (rhs) CNODE_ADD_CHILD(n, rhs);
            lhs = n;
        } else {
            break;
        }
    }
    return lhs;
}

STATIC PCNODE parse_expr() { return parse_expr_prec(0); }

/* ── Statement parsing ────────────────────────────────────────────────────── */

STATIC PCNODE parse_stmt() {
    PCOMP_TOK t = PEEK();
    if (!t) return NULLPTR;
    U32 sl = t->line, sc = t->col;

    /* Block */
    if (MATCH(CTOK_LBRACE)) return parse_block();

    /* if */
    if (MATCH(CTOK_KW_IF)) {
        ADV(); EXPECT(CTOK_LPAREN);
        PCNODE cond = parse_expr();
        EXPECT(CTOK_RPAREN);
        PCNODE body = parse_stmt();
        PCNODE n = CNODE_NEW(CNODE_IF, sl, sc);
        if (cond) CNODE_ADD_CHILD(n, cond);
        if (body) CNODE_ADD_CHILD(n, body);
        if (MATCH(CTOK_KW_ELSE)) { ADV(); PCNODE eb = parse_stmt(); if (eb) CNODE_ADD_CHILD(n, eb); }
        return n;
    }

    /* for */
    if (MATCH(CTOK_KW_FOR)) {
        ADV(); EXPECT(CTOK_LPAREN);
        PCNODE init;
        BOOL isType = FALSE;
        if (MATCH(CTOK_SEMICOLON)) { init = CNODE_NEW(CNODE_EXPR_STMT, sl, sc); }
        else {
            /* Check if init starts with a type keyword */
            COMP_TOK_TYPE ft = PEEK()->type;
            isType = (ft >= CTOK_KW_U8 && ft <= CTOK_KW_PPI32) || ft == CTOK_KW_F32
                   || ft == CTOK_KW_BOOL || ft == CTOK_KW_U0 || ft == CTOK_KW_VOIDPTR
                   || ft == CTOK_KW_VOID;
            if (ft == CTOK_IDENT) {
                SYMBOL *ts = SYM_LOOKUP(PEEK()->txt);
                isType = SYM_IS_TYPE(ts);
            }
            if (isType) {
                /* Parse var decl without consuming final semicolon */
                COMP_TYPE vt = parse_type();
                PCOMP_TOK vn = EXPECT(CTOK_IDENT);
                if (!vn) init = NULLPTR;
                else {
                    init = CNODE_NEW(CNODE_VAR_DECL, sl, sc);
                    init->txt = AC_STRDUP(vn->txt);
                    init->dtype = vt;
                    SYMBOL *vs = SYM_ADD(vn->txt, SYM_VARIABLE);
                    vs->type = vt;
                    vs->is_global = FALSE;
                    if (MATCH(CTOK_ASSIGN)) { ADV(); PCNODE ie = parse_expr(); if (ie) CNODE_ADD_CHILD(init, ie); }
                    if (MATCH(CTOK_SEMICOLON)) ADV(); /* consume the ; */
                }
            } else {
                init = parse_expr();
            }
        }
        if (!isType) EXPECT(CTOK_SEMICOLON);
        PCNODE cond = MATCH(CTOK_SEMICOLON) ? CNODE_INT(1, sl, sc) : parse_expr();
        EXPECT(CTOK_SEMICOLON);
        PCNODE step = MATCH(CTOK_RPAREN) ? NULLPTR : parse_expr();
        EXPECT(CTOK_RPAREN);
        PCNODE body = parse_stmt();
        PCNODE n = CNODE_NEW(CNODE_FOR, sl, sc);
        if (init) CNODE_ADD_CHILD(n, init);
        if (cond) CNODE_ADD_CHILD(n, cond);
        if (step) CNODE_ADD_CHILD(n, step);
        if (body) CNODE_ADD_CHILD(n, body);
        return n;
    }

    /* while */
    if (MATCH(CTOK_KW_WHILE)) {
        ADV(); EXPECT(CTOK_LPAREN);
        PCNODE cond = parse_expr();
        EXPECT(CTOK_RPAREN);
        PCNODE body = parse_stmt();
        PCNODE n = CNODE_NEW(CNODE_WHILE, sl, sc);
        if (cond) CNODE_ADD_CHILD(n, cond);
        if (body) CNODE_ADD_CHILD(n, body);
        return n;
    }

    /* do-while */
    if (MATCH(CTOK_KW_DO)) {
        ADV();
        PCNODE body = parse_stmt();
        EXPECT(CTOK_KW_WHILE); EXPECT(CTOK_LPAREN);
        PCNODE cond = parse_expr();
        EXPECT(CTOK_RPAREN); EXPECT(CTOK_SEMICOLON);
        PCNODE n = CNODE_NEW(CNODE_DO_WHILE, sl, sc);
        if (body) CNODE_ADD_CHILD(n, body);
        if (cond) CNODE_ADD_CHILD(n, cond);
        return n;
    }

    /* switch */
    if (MATCH(CTOK_KW_SWITCH)) {
        ADV(); EXPECT(CTOK_LPAREN);
        PCNODE expr = parse_expr();
        EXPECT(CTOK_RPAREN);
        PCNODE n = CNODE_NEW(CNODE_SWITCH, sl, sc);
        if (expr) CNODE_ADD_CHILD(n, expr);
        EXPECT(CTOK_LBRACE);
        while (PEEK() && !MATCH(CTOK_RBRACE) && !MATCH(CTOK_EOF)) {
            if (MATCH(CTOK_KW_CASE)) {
                ADV();
                PCNODE cv = parse_expr();
                EXPECT(CTOK_COLON);
                PCNODE cn = CNODE_NEW(CNODE_CASE, cv ? cv->line : sl, cv ? cv->col : sc);
                if (cv) CNODE_ADD_CHILD(cn, cv);
                while (PEEK() && !MATCH(CTOK_KW_CASE) && !MATCH(CTOK_KW_DEFAULT) && !MATCH(CTOK_RBRACE))
                    CNODE_ADD_CHILD(cn, parse_stmt());
                CNODE_ADD_CHILD(n, cn);
            } else if (MATCH(CTOK_KW_DEFAULT)) {
                ADV(); EXPECT(CTOK_COLON);
                PCNODE dn = CNODE_NEW(CNODE_DEFAULT, sl, sc);
                while (PEEK() && !MATCH(CTOK_KW_CASE) && !MATCH(CTOK_RBRACE))
                    CNODE_ADD_CHILD(dn, parse_stmt());
                CNODE_ADD_CHILD(n, dn);
            } else { ADV(); }
        }
        EXPECT(CTOK_RBRACE);
        return n;
    }

    /* break / continue */
    if (MATCH(CTOK_KW_BREAK))    { ADV(); EXPECT(CTOK_SEMICOLON); return CNODE_NEW(CNODE_BREAK, sl, sc); }
    if (MATCH(CTOK_KW_CONTINUE)) { ADV(); EXPECT(CTOK_SEMICOLON); return CNODE_NEW(CNODE_CONTINUE, sl, sc); }

    /* return */
    if (MATCH(CTOK_KW_RETURN)) {
        ADV();
        PCNODE n = CNODE_NEW(CNODE_RETURN, sl, sc);
        if (!MATCH(CTOK_SEMICOLON)) {
            PCNODE e = parse_expr();
            if (e) CNODE_ADD_CHILD(n, e);
        }
        EXPECT(CTOK_SEMICOLON);
        return n;
    }

    /* goto / label */
    if (MATCH(CTOK_KW_GOTO)) {
        ADV(); PCOMP_TOK lbl = EXPECT(CTOK_IDENT); EXPECT(CTOK_SEMICOLON);
        PCNODE n = CNODE_NEW(CNODE_GOTO, sl, sc);
        if (lbl) n->txt = AC_STRDUP(lbl->txt);
        return n;
    }
    if (MATCH(CTOK_IDENT) && pos + 1 < toks->len && toks->toks[pos + 1]->type == CTOK_COLON) {
        PCOMP_TOK lt = ADV(); ADV();
        PCNODE n = CNODE_NEW(CNODE_LABEL, sl, sc);
        n->txt = AC_STRDUP(lt->txt);
        return n;
    }

    /* asm block */
    if (MATCH(CTOK_KW_ASM)) {
        ADV();
        PCNODE n = CNODE_NEW(CNODE_ASM_BLOCK, sl, sc);
        if (MATCH(CTOK_ASM_BODY)) {
            PCOMP_TOK body = ADV();
            if (body->txt) n->txt = AC_STRDUP(body->txt);
        }
        return n;
    }

    /* Variable declaration (type identifier ...) or expression statement */
    BOOL is_typedef_name = FALSE;
    if (MATCH(CTOK_IDENT)) {
        SYMBOL *ts = SYM_LOOKUP(PEEK()->txt);
        if (SYM_IS_TYPE(ts) && pos + 1 < toks->len) {
            /* Only enter type chain if next token is an identifier (var name),
             * * (pointer), or it's a function call like TYPE(...) */
            COMP_TOK_TYPE next = toks->toks[pos + 1]->type;
            is_typedef_name = (next == CTOK_IDENT || next == CTOK_STAR || next == CTOK_LPAREN);
        }
    }
    if (is_typedef_name || MATCH(CTOK_KW_U8) || MATCH(CTOK_KW_U16) || MATCH(CTOK_KW_U32)
        || MATCH(CTOK_KW_I8) || MATCH(CTOK_KW_I16) || MATCH(CTOK_KW_I32)
        || MATCH(CTOK_KW_F32) || MATCH(CTOK_KW_BOOL) || MATCH(CTOK_KW_U0)
        || MATCH(CTOK_KW_VOIDPTR) || MATCH(CTOK_KW_PU8) || MATCH(CTOK_KW_PU16)
        || MATCH(CTOK_KW_PU32) || MATCH(CTOK_KW_PPU8) || MATCH(CTOK_KW_PPU16)
        || MATCH(CTOK_KW_PPU32) || MATCH(CTOK_KW_PI8) || MATCH(CTOK_KW_PI16)
        || MATCH(CTOK_KW_PI32) || MATCH(CTOK_KW_PPI8) || MATCH(CTOK_KW_PPI16)
        || MATCH(CTOK_KW_PPI32) || MATCH(CTOK_KW_STRUCT) || MATCH(CTOK_KW_UNION)
        || MATCH(CTOK_KW_ENUM) || MATCH(CTOK_KW_VOID)) {
        COMP_TYPE vt = parse_type();
        PCOMP_TOK it = EXPECT(CTOK_IDENT);
        if (!it) { SKIP_TO_SEMI(); return NULLPTR; }

        /* Array declaration? */
        if (MATCH(CTOK_LBRACKET)) {
            ADV();
            PCNODE sz = parse_expr();
            EXPECT(CTOK_RBRACKET);
            PCNODE n = CNODE_NEW(CNODE_VAR_DECL, sl, sc);
            n->txt   = AC_STRDUP(it->txt);
            n->dtype = vt;
            if (sz) CNODE_ADD_CHILD(n, sz);
            SYMBOL *av = SYM_ADD(it->txt, SYM_VARIABLE);
            av->type  = vt;
            av->is_global = !ctx->in_func;
            if (sz && sz->ntype == CNODE_INT_LIT) av->array_size = sz->ival;
            EXPECT(CTOK_SEMICOLON);
            return n;
        }

        /* Check for function declaration */
        if (MATCH(CTOK_LPAREN)) {
            ADV();
            /* Parse parameter list */
            SYMBOL *fs = SYM_ADD(it->txt, SYM_FUNCTION);
            fs->ret_type  = vt;
            fs->is_global = TRUE;
            fs->param_count = 0;
            fs->is_variadic = FALSE;

            if (!MATCH(CTOK_RPAREN)) {
                do {
                    if (MATCH(CTOK_DOT) && pos + 2 < toks->len
                        && toks->toks[pos+1]->type == CTOK_DOT
                        && toks->toks[pos+2]->type == CTOK_DOT) {
                        ADV(); ADV(); ADV(); /* skip ... */
                        fs->is_variadic = TRUE;
                        break;
                    }
                    COMP_TYPE pt = parse_type();
                    if (MATCH(CTOK_IDENT) && fs->param_count < 16) {
                        PCOMP_TOK pn = ADV();
                        fs->param_types[fs->param_count] = pt;
                        fs->param_names[fs->param_count] = AC_STRDUP(pn->txt);
                        /* Also add as variable in symbol table */
                        SYMBOL *pv = SYM_ADD(pn->txt, SYM_VARIABLE);
                        pv->type      = pt;
                        pv->is_global = FALSE;
                        fs->param_count++;
                    }
                } while (MATCH(CTOK_COMMA) && ADV());
            }
            EXPECT(CTOK_RPAREN);

            PCNODE fn = CNODE_NEW(CNODE_FUNC_DECL, sl, sc);
            fn->txt   = AC_STRDUP(it->txt);
            fn->dtype = vt;
            fn->ival  = fs->param_count;

            /* Add param nodes */
            for (U32 p = 0; p < fs->param_count; p++) {
                PCNODE pn = CNODE_NEW(CNODE_PARAM, sl, sc);
                pn->dtype = fs->param_types[p];
                pn->txt   = AC_STRDUP(fs->param_names[p]);
                CNODE_ADD_CHILD(fn, pn);
            }

            if (MATCH(CTOK_SEMICOLON)) {
                /* Forward declaration only */
                ADV();
                fs->is_defined = FALSE;
                return fn;
            }

            /* Function body */
            fs->is_defined = TRUE;
            ctx->in_func   = TRUE;
            ctx->cur_func  = fn;
            PCNODE body = parse_block();
            ctx->in_func  = FALSE;
            if (body) CNODE_ADD_CHILD(fn, body);
            return fn;
        }

        /* Simple variable declaration */
        PCNODE n = CNODE_NEW(CNODE_VAR_DECL, sl, sc);
        n->txt   = AC_STRDUP(it->txt);
        n->dtype = vt;
        SYMBOL *vs = SYM_ADD(it->txt, SYM_VARIABLE);
        vs->type  = vt;
        vs->is_global = !ctx->in_func;
        /* Initializer: = expression */
        if (MATCH(CTOK_ASSIGN)) {
            ADV();
            PCNODE init = parse_expr();
            if (init) CNODE_ADD_CHILD(n, init);
        }
        EXPECT(CTOK_SEMICOLON);
        return n;
    }

    /* Expression statement */
    PCNODE e = parse_expr();
    EXPECT(CTOK_SEMICOLON);
    if (!e) return NULLPTR;
    PCNODE n = CNODE_NEW(CNODE_EXPR_STMT, e->line, e->col);
    CNODE_ADD_CHILD(n, e);
    return n;
}

STATIC PCNODE parse_block() {
    PCOMP_TOK t = EXPECT(CTOK_LBRACE);
    if (!t) return NULLPTR;
    PCNODE n = CNODE_NEW(CNODE_BLOCK, t->line, t->col);
    while (PEEK() && !MATCH(CTOK_RBRACE) && !MATCH(CTOK_EOF)) {
        PCNODE s = parse_stmt();
        if (s) CNODE_ADD_CHILD(n, s);
    }
    EXPECT(CTOK_RBRACE);
    return n;
}

/* ── Top-level declaration parsing ────────────────────────────────────────── */

STATIC PCNODE parse_toplevel() {
    PCOMP_TOK t = PEEK();
    if (!t || MATCH(CTOK_EOF)) return NULLPTR;
    U32 sl = t->line, sc = t->col;

    /* If 'typedef struct/union/enum', consume typedef and delegate to struct handler */
    if (MATCH(CTOK_KW_TYPEDEF) && pos + 1 < toks->len) {
        COMP_TOK_TYPE nt = toks->toks[pos + 1]->type;
        if (nt == CTOK_KW_STRUCT || nt == CTOK_KW_UNION || nt == CTOK_KW_ENUM) {
            ADV(); /* consume 'typedef', fall through to struct/enum handler */
        }
    }

    /* struct / union / enum definition */
    if (MATCH(CTOK_KW_STRUCT) || MATCH(CTOK_KW_UNION) || MATCH(CTOK_KW_ENUM)) {
        BOOL is_union = MATCH(CTOK_KW_UNION);
        BOOL is_enum  = MATCH(CTOK_KW_ENUM);
        ADV();
        PCOMP_TOK nt = EXPECT(CTOK_IDENT);
        if (!nt) return NULLPTR;

        if (is_enum) {
            SYMBOL *es = SYM_ADD(nt->txt, SYM_ENUM);
            es->type = COMP_MAKE_TYPE(CTYPE_ENUM, 0, NULLPTR);
            EXPECT(CTOK_LBRACE);
            U32 ev = 0;
            while (PEEK() && !MATCH(CTOK_RBRACE) && !MATCH(CTOK_EOF)) {
                PCOMP_TOK fn = EXPECT(CTOK_IDENT);
                if (!fn) break;
                if (MATCH(CTOK_ASSIGN)) { ADV(); PCNODE ve = parse_atom(); ev = ve ? ve->ival : 0; }
                SYMBOL *fs = SYM_ADD(fn->txt, SYM_VARIABLE);
                fs->type  = COMP_MAKE_TYPE(CTYPE_ENUM, 0, NULLPTR);
                fs->ival  = ev;
                ev++;
                if (MATCH(CTOK_COMMA)) ADV();
            }
            EXPECT(CTOK_RBRACE);
            /* Handle typedef chain after enum body: } TYPE, *PTYPE; */
            while (MATCH(CTOK_IDENT) || MATCH(CTOK_STAR)) {
                U8 pd = 0;
                while (MATCH(CTOK_STAR)) { ADV(); pd++; }
                if (MATCH(CTOK_IDENT)) {
                    PCOMP_TOK tn = ADV();
                    SYMBOL *ts = SYM_ADD(tn->txt, SYM_TYPEDEF);
                    ts->type = COMP_MAKE_TYPE(CTYPE_ENUM, pd, NULLPTR);
                }
                if (MATCH(CTOK_COMMA)) ADV(); else break;
            }
            EXPECT(CTOK_SEMICOLON);
            return CNODE_NEW(CNODE_ENUM_DECL, sl, sc);
        }

        SYMBOL *ss = SYM_ADD(nt->txt, is_union ? SYM_UNION : SYM_STRUCT);
        ss->total_size = 0;
        ss->field_count = 0;

        if (MATCH(CTOK_LBRACE)) {
            ADV();
            while (PEEK() && !MATCH(CTOK_RBRACE) && !MATCH(CTOK_EOF) && ss->field_count < 64) {
                COMP_TYPE ft = parse_type();
                PCOMP_TOK fn = EXPECT(CTOK_IDENT);
                if (!fn) break;

                /* Optional array dimension: type name[N] */
                U32 count = 1;
                if (MATCH(CTOK_LBRACKET)) {
                    ADV();
                    PCNODE sz = parse_atom();
                    if (sz && sz->ntype == CNODE_INT_LIT && sz->ival > 0) count = sz->ival;
                    EXPECT(CTOK_RBRACKET);
                }

                if (MATCH(CTOK_SEMICOLON)) {
                    ADV();
                    U32 elem = FIELD_TYPE_SIZE(ft);
                    U32 fsz  = elem * count;
                    ss->fields[ss->field_count].name       = AC_STRDUP(fn->txt);
                    ss->fields[ss->field_count].type       = ft;
                    ss->fields[ss->field_count].array_size = count;
                    ss->fields[ss->field_count].offset     = is_union ? 0 : ss->total_size;
                    ss->fields[ss->field_count].size       = fsz;
                    if (is_union) { if (fsz > ss->total_size) ss->total_size = fsz; }
                    else ss->total_size += fsz;
                    ss->field_count++;
                } else { break; }
            }
            EXPECT(CTOK_RBRACE);
        }

        /* Handle typedef: struct _X { ... } TYPE, *PTYPE; */
        while (MATCH(CTOK_IDENT) || MATCH(CTOK_STAR)) {
            /* Skip '*' for pointer typedef */
            U8 pd = 0;
            while (MATCH(CTOK_STAR)) { ADV(); pd++; }
            if (MATCH(CTOK_IDENT)) {
                PCOMP_TOK tn = ADV();
                SYMBOL *ts = SYM_ADD(tn->txt, SYM_TYPEDEF);
                ts->type = COMP_MAKE_TYPE(ss->is_union ? CTYPE_UNION : CTYPE_STRUCT, pd, ss->name);
            }
            if (MATCH(CTOK_COMMA)) ADV(); else break;
        }
        EXPECT(CTOK_SEMICOLON);
        return CNODE_NEW(CNODE_STRUCT_DECL, sl, sc);
    }

    /* typedef (non-struct types) */
    if (MATCH(CTOK_KW_TYPEDEF)) {
        ADV();
        COMP_TYPE tt = parse_type();
        /* Function pointer typedef: typedef RET (*NAME)(PARAMS); */
        if (MATCH(CTOK_LPAREN)) {
            ADV(); /* skip ( */
            /* Check for *name pattern */
            if (MATCH(CTOK_STAR)) {
                ADV(); /* skip * */
                PCOMP_TOK fpname = EXPECT(CTOK_IDENT);
                if (!fpname) { SKIP_TO_SEMI(); return NULLPTR; }
                EXPECT(CTOK_RPAREN); /* skip ) */
                EXPECT(CTOK_LPAREN); /* skip ( */
                /* Parse params */
                U32 pc = 0;
                COMP_TYPE pt[16];
                if (!MATCH(CTOK_RPAREN)) {
                    do {
                        if (pc < 16) { pt[pc] = parse_type(); if (MATCH(CTOK_IDENT)) ADV(); pc++; }
                    } while (MATCH(CTOK_COMMA) && ADV());
                }
                EXPECT(CTOK_RPAREN);
                /* Store function pointer type */
                SYMBOL *ts = SYM_ADD(fpname->txt, SYM_TYPEDEF);
                ts->type = tt; /* store ret type in type, ptr_depth=1 for function ptr */
                ts->param_count = pc;
                AC_MEMCPY(ts->param_types, pt, pc * sizeof(COMP_TYPE));
                ts->kind = SYM_TYPEDEF;
            } else {
                SKIP_TO_SEMI();
            }
            EXPECT(CTOK_SEMICOLON);
            return CNODE_NEW(CNODE_ENUM_DECL, sl, sc);
        }
        PCOMP_TOK tn = EXPECT(CTOK_IDENT);
        if (tn) {
            SYMBOL *ts = SYM_ADD(tn->txt, SYM_TYPEDEF);
            ts->type = tt;
        }
        EXPECT(CTOK_SEMICOLON);
        return CNODE_NEW(CNODE_ENUM_DECL, sl, sc);
    }

    /* static/local variable or function */
    BOOL is_static = FALSE, is_local = FALSE;
    if (MATCH(CTOK_KW_STATIC)) { ADV(); is_static = TRUE; }
    else if (MATCH(CTOK_KW_LOCAL)) { ADV(); is_local = TRUE; }

    /* Check for typedef name or bare tag name as type */
    BOOL is_typedef_name2 = FALSE;
    if (MATCH(CTOK_IDENT)) {
        SYMBOL *ts = SYM_LOOKUP(PEEK()->txt);
        if (SYM_IS_TYPE(ts) && pos + 1 < toks->len) {
            COMP_TOK_TYPE next = toks->toks[pos + 1]->type;
            is_typedef_name2 = (next == CTOK_IDENT || next == CTOK_STAR || next == CTOK_LPAREN);
        }
    }
    if (is_typedef_name2 || MATCH(CTOK_KW_U8) || MATCH(CTOK_KW_U16) || MATCH(CTOK_KW_U32)
        || MATCH(CTOK_KW_I8) || MATCH(CTOK_KW_I16) || MATCH(CTOK_KW_I32)
        || MATCH(CTOK_KW_F32) || MATCH(CTOK_KW_BOOL) || MATCH(CTOK_KW_U0)
        || MATCH(CTOK_KW_VOIDPTR) || MATCH(CTOK_KW_PU8) || MATCH(CTOK_KW_PU16)
        || MATCH(CTOK_KW_PU32) || MATCH(CTOK_KW_PPU8) || MATCH(CTOK_KW_PPU16)
        || MATCH(CTOK_KW_PPU32) || MATCH(CTOK_KW_PI8) || MATCH(CTOK_KW_PI16)
        || MATCH(CTOK_KW_PI32) || MATCH(CTOK_KW_PPI8) || MATCH(CTOK_KW_PPI16)
        || MATCH(CTOK_KW_PPI32) || MATCH(CTOK_KW_STRUCT) || MATCH(CTOK_KW_UNION)
        || MATCH(CTOK_KW_ENUM) || MATCH(CTOK_KW_VOID)) {
        COMP_TYPE vt = parse_type();
        PCOMP_TOK it = EXPECT(CTOK_IDENT);
        if (!it) { SKIP_TO_SEMI(); return NULLPTR; }
        if (MATCH(CTOK_LPAREN)) {
            /* Function */
            ADV();
            SYMBOL *fs = SYM_ADD(it->txt, SYM_FUNCTION);
            fs->ret_type  = vt;
            fs->is_global = is_static;
            fs->param_count = 0;
            fs->is_variadic = FALSE;
            if (!MATCH(CTOK_RPAREN)) {
                do {
                    if (MATCH(CTOK_DOT) && pos + 2 < toks->len
                        && toks->toks[pos+1]->type == CTOK_DOT
                        && toks->toks[pos+2]->type == CTOK_DOT) {
                        ADV(); ADV(); ADV(); fs->is_variadic = TRUE; break;
                    }
                    COMP_TYPE pt = parse_type();
                    if (MATCH(CTOK_IDENT) && fs->param_count < 16) {
                        PCOMP_TOK pn = ADV();
                        fs->param_types[fs->param_count] = pt;
                        fs->param_names[fs->param_count] = AC_STRDUP(pn->txt);
                        /* Also add as variable in symbol table */
                        SYMBOL *pv = SYM_ADD(pn->txt, SYM_VARIABLE);
                        pv->type      = pt;
                        pv->is_global = FALSE;
                        fs->param_count++;
                    }
                } while (MATCH(CTOK_COMMA) && ADV());
            }
            EXPECT(CTOK_RPAREN);

            PCNODE fn = CNODE_NEW(CNODE_FUNC_DECL, sl, sc);
            fn->txt   = AC_STRDUP(it->txt);
            fn->dtype = vt;
            fn->ival  = fs->param_count;
            for (U32 p = 0; p < fs->param_count; p++) {
                PCNODE pn = CNODE_NEW(CNODE_PARAM, sl, sc);
                pn->dtype = fs->param_types[p];
                pn->txt   = AC_STRDUP(fs->param_names[p]);
                CNODE_ADD_CHILD(fn, pn);
            }
            if (MATCH(CTOK_SEMICOLON)) { ADV(); fs->is_defined = FALSE; return fn; }
            fs->is_defined = TRUE;
            ctx->in_func = TRUE; ctx->cur_func = fn;
            PCNODE body = parse_block();
            ctx->in_func = FALSE;
            if (body) CNODE_ADD_CHILD(fn, body);
            return fn;
        }

        PCNODE n = CNODE_NEW(CNODE_VAR_DECL, sl, sc);
        n->txt = AC_STRDUP(it->txt);
        n->dtype = vt;
        SYMBOL *vs = SYM_ADD(it->txt, SYM_VARIABLE);
        vs->type = vt;
        vs->is_global = TRUE;
        vs->is_file_local = is_local;
        if (is_local) vs->file_scope = ctx->file_scope;
        if (MATCH(CTOK_LBRACKET)) {
            ADV();
            PCNODE size = parse_atom();
            if (size) CNODE_ADD_CHILD(n, size);
            if (size && size->ntype == CNODE_INT_LIT) vs->array_size = size->ival;
            EXPECT(CTOK_RBRACKET);
        }
        if (MATCH(CTOK_ASSIGN)) {
            ADV();
            PCNODE init = parse_expr();
            if (init) CNODE_ADD_CHILD(n, init);
        }
        EXPECT(CTOK_SEMICOLON);
        return n;
    }

    AC_PRINTF("[PARSE] L%u unexpected token at top level\n", sl);
    ADV(); return NULLPTR;
}

/* ── Entry point ───────────────────────────────────────────────────────────── */

PCNODE COMP_PARSE(PCOMP_TOK_ARRAY t, PCOMP_CTX c) {
    if (!t || !c) return NULLPTR;
    toks = t; pos = 0; ctx = c; sym = &c->symtab;
    AC_MEMZERO(sym, sizeof(SYM_TABLE));

    PCNODE root = CNODE_NEW(CNODE_PROGRAM, 0, 0);

    while (PEEK() && !MATCH(CTOK_EOF)) {
        PCNODE n = parse_toplevel();
        if (n) CNODE_ADD_CHILD(root, n);
    }

    return root;
}
