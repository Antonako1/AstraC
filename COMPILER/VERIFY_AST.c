/*
 * COMPILER/VERIFY_AST.c — Semantic verification pass.
 *
 * Walks the AST produced by COMP_PARSE() and checks:
 *   - type compatibility, undefined identifiers, arity, return types.
 * Resolves CNODE_IDENT nodes against the symbol table, setting their dtype.
 */
#include "COMPILER.h"

STATIC PCOMP_CTX ctx;
STATIC SYM_TABLE *sym;

STATIC SYMBOL *V_FIND_SYM(PU8 name) {
    for (U32 i = 0; i < sym->count; i++)
        if (sym->entries[i].name && AC_STRCMP(sym->entries[i].name, name) == 0) {
            if (sym->entries[i].is_file_local && sym->entries[i].file_scope != ctx->file_scope)
                return NULLPTR; /* file-local symbol from another file — invisible */
            return &sym->entries[i];
        }
    return NULLPTR;
}

STATIC BOOL IS_INTEGER(COMP_TYPE t) {
    return (t.base >= CTYPE_U8 && t.base <= CTYPE_I32) || t.base == CTYPE_BOOL;
}

STATIC BOOL IS_INTEGER_TYPE(COMP_TYPE t) {
    return (t.base >= CTYPE_U8 && t.base <= CTYPE_I32) || t.base == CTYPE_BOOL;
}

/* TRUE if the non-negative integer literal `val` fits in type `dst`. */
STATIC BOOL CONST_FITS(COMP_TYPE dst, U32 val) {
    switch (dst.base) {
        case CTYPE_U8:   return val <= 0xFF;
        case CTYPE_I8:   return val <= 0x7F;
        case CTYPE_U16:  return val <= 0xFFFF;
        case CTYPE_I16:  return val <= 0x7FFF;
        case CTYPE_BOOL: return val <= 1;
        default:         return TRUE;   /* U32 / I32 */
    }
}

STATIC BOOL IS_POINTER(COMP_TYPE t) {
    return t.base >= CTYPE_PU8 || t.base == CTYPE_VOIDPTR;
}

STATIC COMP_TYPE STRIP_PTR_TYPE(COMP_TYPE t) {
    if (t.ptr_depth > 0)
        return COMP_MAKE_TYPE(t.base, t.ptr_depth - 1, t.name);
    switch (t.base) {
        case CTYPE_PU8:   return COMP_MAKE_TYPE(CTYPE_U8, 0, NULLPTR);
        case CTYPE_PU16:  return COMP_MAKE_TYPE(CTYPE_U16, 0, NULLPTR);
        case CTYPE_PU32:  return COMP_MAKE_TYPE(CTYPE_U32, 0, NULLPTR);
        case CTYPE_PI8:   return COMP_MAKE_TYPE(CTYPE_I8, 0, NULLPTR);
        case CTYPE_PI16:  return COMP_MAKE_TYPE(CTYPE_I16, 0, NULLPTR);
        case CTYPE_PI32:  return COMP_MAKE_TYPE(CTYPE_I32, 0, NULLPTR);
        case CTYPE_PPU8:  return COMP_MAKE_TYPE(CTYPE_PU8, 0, NULLPTR);
        case CTYPE_PPU16: return COMP_MAKE_TYPE(CTYPE_PU16, 0, NULLPTR);
        case CTYPE_PPU32: return COMP_MAKE_TYPE(CTYPE_PU32, 0, NULLPTR);
        case CTYPE_PPI8:  return COMP_MAKE_TYPE(CTYPE_PI8, 0, NULLPTR);
        case CTYPE_PPI16: return COMP_MAKE_TYPE(CTYPE_PI16, 0, NULLPTR);
        case CTYPE_PPI32: return COMP_MAKE_TYPE(CTYPE_PI32, 0, NULLPTR);
        case CTYPE_VOIDPTR: return COMP_MAKE_TYPE(CTYPE_U8, 0, NULLPTR);
        default: return t;
    }
}

STATIC VOID WARN(PU8 msg, U32 line, U32 col) {
    if (!WARNING(1)) return;   /* respect the --warn level (default: silent) */
    AC_PRINTF("[VERIFY] L%u:%u warning: %s\n", line, col, msg);
    ctx->warnings++;
}

STATIC VOID ERR(PU8 msg, U32 line, U32 col) {
    AC_PRINTF("[VERIFY] L%u:%u error: %s\n", line, col, msg);
    ctx->errors++;
}

STATIC BOOL TYPES_EQUAL(COMP_TYPE a, COMP_TYPE b) {
    return a.base == b.base && a.ptr_depth == b.ptr_depth
           && ((a.name && b.name && AC_STRCMP(a.name, b.name) == 0)
               || (!a.name && !b.name));
}

STATIC COMP_TYPE RESOLVE_TYPE(COMP_TYPE t) {
    if (!t.name) return t;
    SYMBOL *s = V_FIND_SYM(t.name);
    if (!s) return t;
    if (s->kind == SYM_STRUCT || s->kind == SYM_UNION || s->kind == SYM_ENUM) {
        COMP_TYPE rt = COMP_MAKE_TYPE(
            s->kind == SYM_STRUCT ? CTYPE_STRUCT :
            s->kind == SYM_UNION ? CTYPE_UNION : CTYPE_ENUM, t.ptr_depth, NULLPTR);
        return rt;
    }
    return t;
}

STATIC U32 TYPE_SIZE(COMP_TYPE t) {
    t = RESOLVE_TYPE(t);
    if (t.base == CTYPE_STRUCT || t.base == CTYPE_UNION) {
        SYMBOL *s = V_FIND_SYM(t.name ? t.name : (PU8)"");
        return s ? s->total_size : 0;
    }
    return COMP_TYPE_SIZE(t);
}

/* Verify a single node. Returns the resolved type of the expression/subtree. */
STATIC COMP_TYPE VERIFY_NODE(PCNODE n) {
    if (!n) return COMP_MAKE_TYPE(CTYPE_NONE, 0, NULLPTR);

    switch (n->ntype) {
        case CNODE_PROGRAM:
        case CNODE_BLOCK:
            for (U32 i = 0; i < n->child_count; i++) VERIFY_NODE(n->children[i]);
            return COMP_MAKE_TYPE(CTYPE_NONE, 0, NULLPTR);

        case CNODE_FUNC_DECL: {
            SYMBOL *fs = V_FIND_SYM(n->txt);
            if (!fs) { ERR("function not in symbol table", n->line, n->col); break; }
            for (U32 i = 0; i < n->child_count; i++)
                if (n->children[i]->ntype != CNODE_PARAM) VERIFY_NODE(n->children[i]);
            break;
        }

        case CNODE_VAR_DECL: {
            SYMBOL *vs = V_FIND_SYM(n->txt);
            if (!vs) { ERR("variable not in symbol table", n->line, n->col); break; }
            /* Array size child is an INT_LIT; optional initializer follows */
            if (n->child_count > 0 && n->children[0]->ntype == CNODE_INT_LIT) {
                vs->array_size = n->children[0]->ival;
                if (n->child_count > 1) VERIFY_NODE(n->children[1]);
            } else if (n->child_count > 0) {
                VERIFY_NODE(n->children[0]);
            }
            break;
        }

        case CNODE_RETURN: {
            SYMBOL *cf = (ctx->cur_func) ? V_FIND_SYM(ctx->cur_func->txt) : NULLPTR;
            if (n->child_count > 0) {
                COMP_TYPE rt = VERIFY_NODE(n->children[0]);
                if (cf && cf->ret_type.base != CTYPE_U0 && !TYPES_EQUAL(cf->ret_type, rt))
                    WARN("return type mismatch", n->line, n->col);
            }
            break;
        }

        case CNODE_IF:
        case CNODE_WHILE:
            VERIFY_NODE(n->children[0]);
            for (U32 i = 1; i < n->child_count; i++) VERIFY_NODE(n->children[i]);
            break;

        case CNODE_DO_WHILE:
            VERIFY_NODE(n->children[0]);
            VERIFY_NODE(n->children[1]);
            break;

        case CNODE_FOR:
            for (U32 i = 0; i < n->child_count; i++) VERIFY_NODE(n->children[i]);
            break;

        case CNODE_SWITCH:
            VERIFY_NODE(n->children[0]);
            for (U32 i = 1; i < n->child_count; i++) VERIFY_NODE(n->children[i]);
            break;

        case CNODE_CASE:
        case CNODE_DEFAULT:
            for (U32 i = 0; i < n->child_count; i++) VERIFY_NODE(n->children[i]);
            break;

        case CNODE_EXPR_STMT:
            if (n->child_count > 0) VERIFY_NODE(n->children[0]);
            break;

        case CNODE_BREAK:
        case CNODE_CONTINUE:
        case CNODE_GOTO:
        case CNODE_LABEL:
            break;

        case CNODE_ASM_BLOCK:
            /* No verification — literal paste */
            break;

        /* Expressions */
        case CNODE_INT_LIT:
            n->dtype = COMP_MAKE_TYPE(CTYPE_U32, 0, NULLPTR);
            return n->dtype;

        case CNODE_FLOAT_LIT:
            n->dtype = COMP_MAKE_TYPE(CTYPE_F32, 0, NULLPTR);
            return n->dtype;

        case CNODE_STR_LIT:
            n->dtype = COMP_MAKE_TYPE(CTYPE_PU8, 0, NULLPTR);
            return n->dtype;

        case CNODE_CHAR_LIT:
            n->dtype = COMP_MAKE_TYPE(CTYPE_U8, 0, NULLPTR);
            return n->dtype;

        case CNODE_TRUE_LIT:
        case CNODE_FALSE_LIT:
            n->dtype = COMP_MAKE_TYPE(CTYPE_BOOL, 0, NULLPTR);
            return n->dtype;

        case CNODE_NULLPTR:
            n->dtype = COMP_MAKE_TYPE(CTYPE_VOIDPTR, 0, NULLPTR);
            return n->dtype;

        case CNODE_IDENT: {
            SYMBOL *s = V_FIND_SYM(n->txt);
            if (!s) {
                if (n->txt) AC_PRINTF("[VERIFY] L%u undefined symbol '%s'\n", n->line, n->txt);
                ERR("undefined symbol", n->line, n->col);
                return COMP_MAKE_TYPE(CTYPE_NONE, 0, NULLPTR);
            }
            if (s->kind == SYM_VARIABLE || s->kind == SYM_FUNCTION) {
                n->dtype = s->type;
                return n->dtype;
            }
            if (s->kind == SYM_ENUM) { n->dtype = COMP_MAKE_TYPE(CTYPE_ENUM, 0, NULLPTR); return n->dtype; }
            n->dtype = COMP_MAKE_TYPE(CTYPE_NONE, 0, NULLPTR);
            return n->dtype;
        }

        case CNODE_BINARY: {
            COMP_TYPE lt = VERIFY_NODE(n->children[0]);
            COMP_TYPE rt = VERIFY_NODE(n->children[1]);
            if (n->op == CTOK_COMMA) { n->dtype = rt; return rt; }
            if (n->op == CTOK_LT || n->op == CTOK_GT || n->op == CTOK_LE
                || n->op == CTOK_GE || n->op == CTOK_EQ || n->op == CTOK_NEQ
                || n->op == CTOK_AND || n->op == CTOK_OR) {
                n->dtype = COMP_MAKE_TYPE(CTYPE_BOOL, 0, NULLPTR);
                return n->dtype;
            }
            n->dtype = lt.base != CTYPE_NONE ? lt : rt;
            return n->dtype;
        }

        case CNODE_UNARY: {
            COMP_TYPE ct = VERIFY_NODE(n->children[0]);
            n->dtype = ct;
            return ct;
        }

        case CNODE_POSTFIX: {
            COMP_TYPE ct = VERIFY_NODE(n->children[0]);
            n->dtype = ct;
            return ct;
        }

        case CNODE_TERNARY: {
            VERIFY_NODE(n->children[0]);
            COMP_TYPE tt = VERIFY_NODE(n->children[1]);
            n->dtype = tt;
            return tt;
        }

        case CNODE_CALL: {
            SYMBOL *fs = V_FIND_SYM(n->txt);
            if (!fs) {
                if (n->txt) AC_PRINTF("[VERIFY] L%u undefined symbol '%s' in call\n", n->line, n->txt);
                ERR("call to undefined function", n->line, n->col);
                return COMP_MAKE_TYPE(CTYPE_NONE, 0, NULLPTR);
            }
            for (U32 i = 0; i < n->child_count; i++) VERIFY_NODE(n->children[i]);
            n->dtype = (fs->kind == SYM_FUNCTION) ? fs->ret_type : fs->type;
            return n->dtype;
        }

        case CNODE_INDEX: {
            COMP_TYPE bt = VERIFY_NODE(n->children[0]);
            VERIFY_NODE(n->children[1]);
            if (bt.base == CTYPE_NONE)
                n->dtype = COMP_MAKE_TYPE(CTYPE_U32, 0, NULLPTR);
            else if (bt.ptr_depth > 0 || IS_POINTER(bt))
                n->dtype = STRIP_PTR_TYPE(bt);
            else
                n->dtype = bt;   /* array symbol: s->type is already the element type */
            return n->dtype;
        }

        case CNODE_MEMBER:
        case CNODE_ARROW_EXPR: {
            COMP_TYPE base = VERIFY_NODE(n->children[0]);
            BOOL is_arrow = (n->ntype == CNODE_ARROW_EXPR);
            COMP_TYPE st = is_arrow ? STRIP_PTR_TYPE(base) : base;

            PU8 fname = (n->child_count > 1 && n->children[1]) ? n->children[1]->txt : NULLPTR;
            SYMBOL *ss = st.name ? V_FIND_SYM(st.name) : NULLPTR;
            COMP_TYPE ft = COMP_MAKE_TYPE(CTYPE_U32, 0, NULLPTR);
            U32 foff = 0;
            U32 farr = 0;
            if (ss && fname) {
                for (U32 j = 0; j < ss->field_count; j++) {
                    if (ss->fields[j].name && AC_STRCMP(ss->fields[j].name, fname) == 0) {
                        ft   = ss->fields[j].type;
                        foff = ss->fields[j].offset;
                        farr = ss->fields[j].array_size;
                        break;
                    }
                }
            }
            n->dtype      = ft;
            n->ival       = foff;   /* field byte offset, used by codegen */
            n->array_size = farr;   /* >0 if the field is an array */
            return ft;
        }

        case CNODE_ASSIGN: {
            COMP_TYPE lt = VERIFY_NODE(n->children[0]);
            COMP_TYPE rt = VERIFY_NODE(n->children[1]);
            /* Allow assignment through member access (-> or .) without type check */
            if (n->children[0] && (n->children[0]->ntype == CNODE_MEMBER
                || n->children[0]->ntype == CNODE_ARROW_EXPR))
                ; /* skip type check for member access */
            else if (lt.base != rt.base && lt.base != CTYPE_NONE && rt.base != CTYPE_NONE) {
                /* Don't warn for an integer constant that fits in the
                 * destination type (e.g. `U8 c = 32;`). */
                PCNODE rhs = n->children[1];
                BOOL const_fits = IS_INTEGER_TYPE(lt) && IS_INTEGER_TYPE(rt)
                               && (rhs && (rhs->ntype == CNODE_INT_LIT || rhs->ntype == CNODE_CHAR_LIT))
                               && CONST_FITS(lt, rhs->ival);
            if (!const_fits)
                WARN("assignment type mismatch", n->line, n->col);
            }
            n->dtype = lt;
            return lt;
        }

        case CNODE_ADDR: {
            COMP_TYPE ct = VERIFY_NODE(n->children[0]);
            n->dtype = COMP_MAKE_TYPE(ct.base, ct.ptr_depth + 1, ct.name);
            return n->dtype;
        }

        case CNODE_DEREF: {
            COMP_TYPE ct = VERIFY_NODE(n->children[0]);
            if (IS_POINTER(ct))
                n->dtype = STRIP_PTR_TYPE(ct);
            else
                n->dtype = ct;
            return n->dtype;
        }

        case CNODE_SIZEOF_TYPE:
            n->ival = TYPE_SIZE(n->dtype);
            n->dtype = COMP_MAKE_TYPE(CTYPE_U32, 0, NULLPTR);
            return n->dtype;

        case CNODE_SIZEOF_EXPR: {
            COMP_TYPE st = VERIFY_NODE(n->children[0]);
            n->ival = TYPE_SIZE(st);
            n->dtype = COMP_MAKE_TYPE(CTYPE_U32, 0, NULLPTR);
            return n->dtype;
        }

        case CNODE_CAST:
            VERIFY_NODE(n->children[0]);
            return n->dtype;

        case CNODE_PARAM:
            return n->dtype;

        default:
            break;
    }
    return COMP_MAKE_TYPE(CTYPE_NONE, 0, NULLPTR);
}

BOOL COMP_VERIFY(PCNODE root, PCOMP_CTX c) {
    if (!root || !c) return FALSE;
    ctx = c; sym = &c->symtab;
    ctx->errors   = 0;
    ctx->warnings = 0;

    VERIFY_NODE(root);

    if (ctx->errors > 0) {
        AC_PRINTF("[VERIFY] %u error(s), %u warning(s)\n", ctx->errors, ctx->warnings);
        return FALSE;
    }
    if (ctx->verbose)
        AC_PRINTF("[VERIFY] %u warning(s)\n", ctx->warnings);
    return TRUE;
}
