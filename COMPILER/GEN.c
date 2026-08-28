/*
 * COMPILER/GEN.c — x86 32-bit assembly code generator.
 *
 * Emits ASTRAC assembler (.AS) source from the verified AST.
 * ABI: cdecl — arguments pushed right-to-left, return value in EAX.
 * Stack frame: PUSH EBP / MOV EBP, ESP on every function.
 */
#include "COMPILER.h"
#include "../ASSEMBLER/ASSEMBLER.h"

STATIC PCOMP_CTX ctx;
STATIC SYM_TABLE *sym;
STATIC FILE      *outf;

STATIC U32 new_label()     { return ++ctx->label_counter; }
STATIC VOID emit(PU8 s)    { AC_FPRINTF(outf, "%s\n", s); }
STATIC I32  local_offset;  /* growing negative for local vars */

/* Helpers for asm block variable substitution */
STATIC BOOL II_START(U8 c) { return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='_'; }
STATIC BOOL II_CONT(U8 c)  { return II_START(c)||(c>='0'&&c<='9'); }
STATIC VOID II_UPPER(PU8 s, U32 n) { for(U32 i=0;i<n;i++) if(s[i]>='a'&&s[i]<='z') s[i]-=32; }

STATIC SYMBOL *FIND_SYM(PU8 name) {
    for (U32 i = 0; i < sym->count; i++)
        if (sym->entries[i].name && AC_STRCMP(sym->entries[i].name, name) == 0)
            return &sym->entries[i];
    return NULLPTR;
}

/* ── Expression codegen — result in EAX ──────────────────────────────────── */

STATIC VOID GEN_EXPR(PCNODE n);
STATIC VOID GEN_MEMBER_ADDR(PCNODE n);
STATIC VOID GEN_LVALUE_ADDR(PCNODE n);

STATIC VOID GEN_LITERAL(PCNODE n) {
    switch (n->ntype) {
        case CNODE_INT_LIT:
            AC_FPRINTF(outf, "    MOV EAX, 0x%X\n", n->ival);
            break;
        case CNODE_FLOAT_LIT:
            AC_FPRINTF(outf, "    MOV EAX, 0x%X\n", (U32)n->fval);
            break;
        case CNODE_STR_LIT:
            AC_FPRINTF(outf, "    MOV EAX, _str%u\n", n->ival);
            break;
        case CNODE_TRUE_LIT:
            emit("    MOV EAX, 1");
            break;
        case CNODE_FALSE_LIT:
            emit("    MOV EAX, 0");
            break;
        case CNODE_NULLPTR:
            emit("    XOR EAX, EAX");
            break;
        default:
            emit("    XOR EAX, EAX");
            break;
    }
}

STATIC VOID GEN_IDENT(PCNODE n) {
    SYMBOL *s = FIND_SYM(n->txt);
    if (!s) { emit("    XOR EAX, EAX"); return; }
    /* Array names decay to pointer-to-first-element (address, not value) */
    if (s->array_size > 0) {
        if (s->is_global)
            AC_FPRINTF(outf, "    LEA EAX, [%s]\n", s->name);
        else if ((I32)s->offset == 0)
            AC_FPRINTF(outf, "    LEA EAX, [EBP]\n");
        else if ((I32)s->offset > 0)
            AC_FPRINTF(outf, "    LEA EAX, [EBP+%u]\n", s->offset);
        else
            AC_FPRINTF(outf, "    LEA EAX, [EBP-%u]\n", (U32)(-(I32)s->offset));
        return;
    }
    {
        U32 es = COMP_TYPE_SIZE(s->type);
        PU8 op = (es == 1) ? "MOVZX EAX, BYTE" : (es == 2) ? "MOVZX EAX, WORD" : "MOV EAX,";
        if (s->is_global)
            AC_FPRINTF(outf, "    %s [%s]\n", op, s->name);
        else if ((I32)s->offset == 0)
            AC_FPRINTF(outf, "    %s [EBP]\n", op);
        else if ((I32)s->offset > 0)
            AC_FPRINTF(outf, "    %s [EBP+%u]\n", op, s->offset);
        else
            AC_FPRINTF(outf, "    %s [EBP-%u]\n", op, (U32)(-(I32)s->offset));
    }
}

STATIC VOID GEN_ARR_BASE(PCNODE base);

STATIC VOID GEN_ASSIGN(PCNODE n) {
    PCNODE lhs = n->children[0];
    U32 es;
    SYMBOL *s;

    if (n->op != CTOK_ASSIGN) {
        /* Compound assignment: read LHS value, add RHS, store back */
        GEN_EXPR(lhs);
        emit("    PUSH EAX");
        GEN_EXPR(n->children[1]);
        emit("    MOV EBX, EAX");
        emit("    POP EAX");
        switch (n->op) {
            case CTOK_PLUS_ASSIGN:  emit("    ADD EAX, EBX"); break;
            case CTOK_MINUS_ASSIGN: emit("    SUB EAX, EBX"); break;
            case CTOK_STAR_ASSIGN:  emit("    IMUL EAX, EBX"); break;
            case CTOK_SLASH_ASSIGN:
                emit("    XOR EDX, EDX");
                emit("    DIV EBX"); break;
            case CTOK_PCT_ASSIGN:
                emit("    XOR EDX, EDX");
                emit("    DIV EBX");
                emit("    MOV EAX, EDX"); break;
            case CTOK_AMP_ASSIGN:  emit("    AND EAX, EBX"); break;
            case CTOK_PIPE_ASSIGN: emit("    OR EAX, EBX"); break;
            case CTOK_CARET_ASSIGN: emit("    XOR EAX, EBX"); break;
            case CTOK_SHL_ASSIGN:
                emit("    MOV ECX, EBX");
                emit("    SHL EAX, CL"); break;
            case CTOK_SHR_ASSIGN:
                emit("    MOV ECX, EBX");
                emit("    SHR EAX, CL"); break;
            default: break;
        }
        emit("    PUSH EAX");
    } else {
        GEN_EXPR(n->children[1]);
        emit("    PUSH EAX");
    }

    /* Common store */
    if (lhs->ntype == CNODE_IDENT) {
        s = FIND_SYM(lhs->txt);
        if (!s) { emit("    POP EAX"); return; }
        emit("    POP EAX");
        es = COMP_TYPE_SIZE(s->type);
        if (es == 1) {
            if (s->is_global)
                AC_FPRINTF(outf, "    MOV [%s], AL\n", s->name);
            else if ((I32)s->offset == 0)
                AC_FPRINTF(outf, "    MOV [EBP], AL\n");
            else if ((I32)s->offset > 0)
                AC_FPRINTF(outf, "    MOV [EBP+%u], AL\n", s->offset);
            else
                AC_FPRINTF(outf, "    MOV [EBP-%u], AL\n", (U32)(-(I32)s->offset));
        } else if (es == 2) {
            if (s->is_global)
                AC_FPRINTF(outf, "    MOV [%s], AX\n", s->name);
            else if ((I32)s->offset == 0)
                AC_FPRINTF(outf, "    MOV [EBP], AX\n");
            else if ((I32)s->offset > 0)
                AC_FPRINTF(outf, "    MOV [EBP+%u], AX\n", s->offset);
            else
                AC_FPRINTF(outf, "    MOV [EBP-%u], AX\n", (U32)(-(I32)s->offset));
        } else {
            if (s->is_global)
                AC_FPRINTF(outf, "    MOV [%s], EAX\n", s->name);
            else if ((I32)s->offset == 0)
                AC_FPRINTF(outf, "    MOV [EBP], EAX\n");
            else if ((I32)s->offset > 0)
                AC_FPRINTF(outf, "    MOV [EBP+%u], EAX\n", s->offset);
            else
                AC_FPRINTF(outf, "    MOV [EBP-%u], EAX\n", (U32)(-(I32)s->offset));
        }
    } else if (lhs->ntype == CNODE_DEREF) {
        GEN_EXPR(lhs->children[0]);
        es = COMP_TYPE_SIZE(lhs->dtype);
        emit("    POP EBX");
        if (es == 1)      emit("    MOV [EAX], BL");
        else if (es == 2)  emit("    MOV [EAX], BX");
        else                emit("    MOV [EAX], EBX");
    } else if (lhs->ntype == CNODE_INDEX) {
        GEN_ARR_BASE(lhs->children[0]);
        emit("    PUSH EAX");
        GEN_EXPR(lhs->children[1]);
        emit("    POP EBX");
        es = COMP_TYPE_SIZE(lhs->dtype);
        if (es == 1)
            emit("    LEA EAX, [EBX + EAX]");
        else if (es == 2)
            emit("    LEA EAX, [EBX + EAX*2]");
        else
            emit("    LEA EAX, [EBX + EAX*4]");
        emit("    POP EBX");
        if (es == 1)      emit("    MOV [EAX], BL");
        else if (es == 2)  emit("    MOV [EAX], BX");
        else                emit("    MOV [EAX], EBX");
    } else {
        emit("    POP EAX");
        emit("    XOR EAX, EAX");
    }
}

STATIC VOID GEN_BINOP(PCNODE n) {
    PCNODE l = n->children[0], r = n->children[1];
    GEN_EXPR(l);
    emit("    PUSH EAX");
    GEN_EXPR(r);
    emit("    MOV EBX, EAX");
    emit("    POP EAX");
    switch (n->op) {
        case CTOK_PLUS: emit("    ADD EAX, EBX"); break;
        case CTOK_MINUS: emit("    SUB EAX, EBX"); break;
        case CTOK_STAR: emit("    IMUL EAX, EBX"); break;
        case CTOK_SLASH:
            emit("    XOR EDX, EDX");
            emit("    DIV EBX"); break;
        case CTOK_PERCENT:
            emit("    XOR EDX, EDX");
            emit("    DIV EBX");
            emit("    MOV EAX, EDX"); break;
        case CTOK_AMP: emit("    AND EAX, EBX"); break;
        case CTOK_PIPE: emit("    OR EAX, EBX"); break;
        case CTOK_CARET: emit("    XOR EAX, EBX"); break;
        case CTOK_SHL: emit("    MOV ECX, EBX"); emit("    SHL EAX, CL"); break;
        case CTOK_SHR: emit("    MOV ECX, EBX"); emit("    SHR EAX, CL"); break;

        /* Relational: set EAX=1/0 */
        case CTOK_EQ: emit("    CMP EAX, EBX\n    SETE AL\n    MOVZX EAX, AL"); break;
        case CTOK_NEQ: emit("    CMP EAX, EBX\n    SETNE AL\n    MOVZX EAX, AL"); break;
        case CTOK_LT:  emit("    CMP EAX, EBX\n    SETL AL\n    MOVZX EAX, AL"); break;
        case CTOK_GT:  emit("    CMP EAX, EBX\n    SETG AL\n    MOVZX EAX, AL"); break;
        case CTOK_LE:  emit("    CMP EAX, EBX\n    SETLE AL\n    MOVZX EAX, AL"); break;
        case CTOK_GE:  emit("    CMP EAX, EBX\n    SETGE AL\n    MOVZX EAX, AL"); break;
        case CTOK_COMMA: emit("    MOV EAX, EBX"); break;
        default: break;
    }
}

STATIC VOID GEN_UNARY(PCNODE n) {
    GEN_EXPR(n->children[0]);
    switch (n->op) {
        case CTOK_MINUS: emit("    NEG EAX"); break;
        case CTOK_TILDE: emit("    NOT EAX"); break;
        case CTOK_NOT:
            emit("    TEST EAX, EAX");
            emit("    SETE AL");
            emit("    MOVZX EAX, AL");
            break;
        default: break;
    }
}

STATIC VOID GEN_CALL(PCNODE n) {
    /* Push args right-to-left */
    for (I32 i = (I32)n->child_count - 1; i >= 0; i--) {
        GEN_EXPR(n->children[i]);
        emit("    PUSH EAX");
    }
    /* Check if target is a function pointer variable */
    SYMBOL *cs = FIND_SYM(n->txt);
    if (n->txt && cs && cs->kind == SYM_VARIABLE) {
        /* Indirect call through function pointer */
        if (cs->is_global)
            AC_FPRINTF(outf, "    CALL [%s]\n", cs->name);
        else if ((I32)cs->offset == 0)
            AC_FPRINTF(outf, "    CALL [EBP]\n");
        else if ((I32)cs->offset > 0)
            AC_FPRINTF(outf, "    CALL [EBP+%u]\n", cs->offset);
        else
            AC_FPRINTF(outf, "    CALL [EBP-%u]\n", (U32)(-(I32)cs->offset));
    } else {
        AC_FPRINTF(outf, "    CALL _%s\n", n->txt ? n->txt : (PU8)"_unknown");
    }
    /* Clean stack (cdecl: caller cleans) */
    if (n->child_count > 0)
        AC_FPRINTF(outf, "    ADD ESP, %u\n", n->child_count * 4);
}

STATIC VOID GEN_ADDR(PCNODE n) {
    PCNODE target = n->children[0];
    if (target->ntype == CNODE_IDENT) {
        SYMBOL *s = FIND_SYM(target->txt);
        if (!s) { emit("    XOR EAX, EAX"); return; }
        if (s->kind == SYM_FUNCTION)
            AC_FPRINTF(outf, "    LEA EAX, [_%s]\n", s->name);
        else if (s->is_global)
            AC_FPRINTF(outf, "    LEA EAX, [%s]\n", s->name);
        else if ((I32)s->offset == 0)
            AC_FPRINTF(outf, "    LEA EAX, [EBP]\n");
        else if ((I32)s->offset > 0)
            AC_FPRINTF(outf, "    LEA EAX, [EBP+%u]\n", s->offset);
        else
            AC_FPRINTF(outf, "    LEA EAX, [EBP-%u]\n", (U32)(-(I32)s->offset));
    } else {
        GEN_EXPR(target);
    }
}

STATIC BOOL IS_PTR_DTYPE(COMP_TYPE t) {
    if (t.ptr_depth > 0) return TRUE;
    switch (t.base) {
        case CTYPE_PU8:  case CTYPE_PU16: case CTYPE_PU32:
        case CTYPE_PPU8: case CTYPE_PPU16: case CTYPE_PPU32:
        case CTYPE_PI8:  case CTYPE_PI16: case CTYPE_PI32:
        case CTYPE_PPI8: case CTYPE_PPI16: case CTYPE_PPI32:
        case CTYPE_VOIDPTR:
            return TRUE;
        default:
            return FALSE;
    }
}

/* A pointer-typed scalar variable (e.g. `PU8 p`) holds the buffer address
 * in its VALUE, so `p[i]` must load that value first (MOV).  An array-typed
 * variable (e.g. `U8 buf[N]`) IS the buffer: its storage address is the
 * array base, so `buf[i]` must take that address (LEA). */
STATIC BOOL IS_PTR_VAR_SYMBOL(SYMBOL *s) {
    if (!s || s->kind != SYM_VARIABLE) return FALSE;
    if (s->array_size > 0) return FALSE;     /* declared array → address-of */
    return IS_PTR_DTYPE(s->type);
}

STATIC VOID GEN_ARR_BASE(PCNODE base) {
    if (base->ntype == CNODE_IDENT) {
        SYMBOL *s = FIND_SYM(base->txt);
        if (!s) { emit("    XOR EAX, EAX"); return; }
        BOOL use_value = IS_PTR_VAR_SYMBOL(s);
        if (s->is_global)
            AC_FPRINTF(outf, "    %s EAX, [%s]\n", use_value ? "MOV" : "LEA", s->name);
        else if ((I32)s->offset == 0)
            AC_FPRINTF(outf, "    %s EAX, [EBP]\n", use_value ? "MOV" : "LEA");
        else if ((I32)s->offset > 0)
            AC_FPRINTF(outf, "    %s EAX, [EBP+%u]\n", use_value ? "MOV" : "LEA", s->offset);
        else
            AC_FPRINTF(outf, "    %s EAX, [EBP-%u]\n", use_value ? "MOV" : "LEA",
                       (U32)(-(I32)s->offset));
    } else {
        GEN_EXPR(base);
    }
}

STATIC VOID GEN_DEREF(PCNODE n) {
    GEN_EXPR(n->children[0]);
    U32 elem_size = COMP_TYPE_SIZE(n->dtype);
    if (elem_size == 1)
        emit("    MOVZX EAX, BYTE [EAX]");
    else if (elem_size == 2)
        emit("    MOVZX EAX, WORD [EAX]");
    else
        emit("    MOV EAX, [EAX]");
}

/* Compute the address of a struct member into EAX. */
STATIC VOID GEN_MEMBER_ADDR(PCNODE n) {
    PCNODE base = n->children[0];
    BOOL is_arrow = (n->ntype == CNODE_ARROW_EXPR);
    if (is_arrow)
        GEN_EXPR(base);          /* base is a pointer — load the address */
    else
        GEN_LVALUE_ADDR(base);   /* base is a value — take its address */
    if (n->ival)
        AC_FPRINTF(outf, "    ADD EAX, %u\n", n->ival);
}

/* Compute the address of an lvalue into EAX. */
STATIC VOID GEN_LVALUE_ADDR(PCNODE n) {
    if (!n) { emit("    XOR EAX, EAX"); return; }
    switch (n->ntype) {
        case CNODE_IDENT: {
            SYMBOL *s = FIND_SYM(n->txt);
            if (!s) { emit("    XOR EAX, EAX"); return; }
            if (s->kind == SYM_FUNCTION)
                AC_FPRINTF(outf, "    LEA EAX, [_%s]\n", s->name);
            else if (s->is_global)
                AC_FPRINTF(outf, "    LEA EAX, [%s]\n", s->name);
            else if ((I32)s->offset == 0)
                AC_FPRINTF(outf, "    LEA EAX, [EBP]\n");
            else if ((I32)s->offset > 0)
                AC_FPRINTF(outf, "    LEA EAX, [EBP+%u]\n", s->offset);
            else
                AC_FPRINTF(outf, "    LEA EAX, [EBP-%u]\n", (U32)(-(I32)s->offset));
            return;
        }
        case CNODE_INDEX: {
            GEN_ARR_BASE(n->children[0]);
            emit("    PUSH EAX");
            GEN_EXPR(n->children[1]);
            emit("    POP EBX");
            U32 es = COMP_TYPE_SIZE(n->dtype);
            if (es == 1)      emit("    LEA EAX, [EBX + EAX]");
            else if (es == 2) emit("    LEA EAX, [EBX + EAX*2]");
            else              emit("    LEA EAX, [EBX + EAX*4]");
            return;
        }
        case CNODE_MEMBER:
        case CNODE_ARROW_EXPR:
            GEN_MEMBER_ADDR(n);
            return;
        case CNODE_DEREF:
            GEN_EXPR(n->children[0]);   /* pointer value is the address */
            return;
        default:
            emit("    XOR EAX, EAX");
            return;
    }
}

/* Load (or take the address of) a struct/union member. */
STATIC VOID GEN_MEMBER(PCNODE n) {
    GEN_MEMBER_ADDR(n);
    COMP_TYPE ft = n->dtype;
    /* A struct/union member is an aggregate, and an array field decays to a
     * pointer — in both cases the address is the result. */
    if (ft.base == CTYPE_STRUCT || ft.base == CTYPE_UNION || n->array_size > 0)
        return;
    U32 es = COMP_TYPE_SIZE(ft);
    if (es == 1)      emit("    MOVZX EAX, BYTE [EAX]");
    else if (es == 2) emit("    MOVZX EAX, WORD [EAX]");
    else              emit("    MOV EAX, [EAX]");
}

STATIC VOID GEN_INDEX(PCNODE n) {
    GEN_ARR_BASE(n->children[0]);  /* base address → EAX */
    emit("    PUSH EAX");
    GEN_EXPR(n->children[1]);  /* index → EAX */
    emit("    POP EBX");
    U32 elem_size = COMP_TYPE_SIZE(n->dtype);
    if (elem_size == 1)
        emit("    MOVZX EAX, BYTE [EBX + EAX]");
    else if (elem_size == 2)
        emit("    MOVZX EAX, WORD [EBX + EAX*2]");
    else
        emit("    MOV EAX, [EBX + EAX*4]");
}

STATIC VOID GEN_CAST(PCNODE n) {
    GEN_EXPR(n->children[0]);
}

STATIC VOID GEN_SIZEOF(PCNODE n) {
    AC_FPRINTF(outf, "    MOV EAX, %u\n", n->ival);
}

/* Increment amount for pointer ++/-- (element size) */
STATIC U32 PTR_INC(COMP_TYPE t) {
    if (t.ptr_depth > 0) return 4;
    switch (t.base) {
        case CTYPE_PU8: case CTYPE_PI8: return 1;
        case CTYPE_PU16: case CTYPE_PI16: return 2;
        case CTYPE_PU32: case CTYPE_PI32: return 4;
        default: return 1;
    }
}

STATIC VOID GEN_POSTFIX(PCNODE n) {
    if (n->op != CTOK_PLUSPLUS && n->op != CTOK_MINUSMINUS) { emit("    XOR EAX, EAX"); return; }
    PCNODE target = n->children[0];
    U32 inc = 1;
    if (target->dtype.base >= CTYPE_PU8 || target->dtype.base == CTYPE_VOIDPTR || target->dtype.ptr_depth > 0)
        inc = PTR_INC(target->dtype);
    if (target->ntype == CNODE_IDENT) {
        SYMBOL *s = FIND_SYM(target->txt);
        if (!s) { emit("    XOR EAX, EAX"); return; }
        if (s->is_global)
            AC_FPRINTF(outf, "    MOV EAX, [%s]\n", s->name);
        else if ((I32)s->offset == 0)
            AC_FPRINTF(outf, "    MOV EAX, [EBP]\n");
        else if ((I32)s->offset > 0)
            AC_FPRINTF(outf, "    MOV EAX, [EBP+%u]\n", s->offset);
        else
            AC_FPRINTF(outf, "    MOV EAX, [EBP-%u]\n", (U32)(-(I32)s->offset));
        emit("    PUSH EAX");
        if (n->op == CTOK_PLUSPLUS)
            AC_FPRINTF(outf, "    ADD EAX, %u\n", inc);
        else
            AC_FPRINTF(outf, "    SUB EAX, %u\n", inc);
        if (s->is_global)
            AC_FPRINTF(outf, "    MOV [%s], EAX\n", s->name);
        else if ((I32)s->offset == 0)
            AC_FPRINTF(outf, "    MOV [EBP], EAX\n");
        else if ((I32)s->offset > 0)
            AC_FPRINTF(outf, "    MOV [EBP+%u], EAX\n", s->offset);
        else
            AC_FPRINTF(outf, "    MOV [EBP-%u], EAX\n", (U32)(-(I32)s->offset));
        emit("    POP EAX");
    } else {
        emit("    XOR EAX, EAX");
    }
}

STATIC VOID GEN_EXPR(PCNODE n) {
    if (!n) { emit("    XOR EAX, EAX"); return; }
    switch (n->ntype) {
        case CNODE_INT_LIT: case CNODE_FLOAT_LIT: case CNODE_STR_LIT:
        case CNODE_TRUE_LIT: case CNODE_FALSE_LIT:
        case CNODE_NULLPTR: case CNODE_CHAR_LIT:
            GEN_LITERAL(n); break;
        case CNODE_IDENT:    GEN_IDENT(n); break;
        case CNODE_BINARY:   GEN_BINOP(n); break;
        case CNODE_UNARY:    GEN_UNARY(n); break;
        case CNODE_CALL:     GEN_CALL(n); break;
        case CNODE_ADDR:     GEN_ADDR(n); break;
        case CNODE_DEREF:    GEN_DEREF(n); break;
        case CNODE_INDEX:    GEN_INDEX(n); break;
        case CNODE_MEMBER:
        case CNODE_ARROW_EXPR: GEN_MEMBER(n); break;
        case CNODE_TERNARY: {
            U32 le = new_label(), lelse = new_label();
            GEN_EXPR(n->children[0]);
            AC_FPRINTF(outf, "    TEST EAX, EAX\n    JZ __lbl%u\n", lelse);
            if (n->child_count > 1) GEN_EXPR(n->children[1]);
            AC_FPRINTF(outf, "    JMP __lbl%u\n__lbl%u:\n", le, lelse);
            if (n->child_count > 2) GEN_EXPR(n->children[2]);
            AC_FPRINTF(outf, "__lbl%u:\n", le);
            break;
        }
        case CNODE_POSTFIX:  GEN_POSTFIX(n); break;
        case CNODE_ASSIGN:   GEN_ASSIGN(n); break;
        case CNODE_CAST:     GEN_CAST(n); break;
        case CNODE_SIZEOF_TYPE:
        case CNODE_SIZEOF_EXPR: GEN_SIZEOF(n); break;
        default: emit("    XOR EAX, EAX"); break;
    }
}

/* ── Statement codegen ────────────────────────────────────────────────────── */

STATIC VOID GEN_STMT(PCNODE n);

STATIC VOID GEN_BLOCK(PCNODE n) {
    for (U32 i = 0; i < n->child_count; i++) GEN_STMT(n->children[i]);
}

STATIC VOID GEN_IF(PCNODE n) {
    U32 lbl_else = new_label(), lbl_end = new_label();
    GEN_EXPR(n->children[0]);
    AC_FPRINTF(outf, "    TEST EAX, EAX\n    JZ __lbl%u\n", lbl_else);
    if (n->child_count > 1) GEN_STMT(n->children[1]);
    AC_FPRINTF(outf, "    JMP __lbl%u\n__lbl%u:\n", lbl_end, lbl_else);
    if (n->child_count > 2) GEN_STMT(n->children[2]);
    AC_FPRINTF(outf, "__lbl%u:\n", lbl_end);
}

STATIC VOID GEN_WHILE(PCNODE n) {
    U32 lbl_start = new_label(), lbl_end = new_label();
    U32 old_top = ctx->loop_label_stack_top;
    ctx->loop_label_stack[ctx->loop_label_stack_top++] = lbl_end;
    ctx->loop_label_stack[ctx->loop_label_stack_top++] = lbl_start;
    AC_FPRINTF(outf, "__lbl%u:\n", lbl_start);
    GEN_EXPR(n->children[0]);
    AC_FPRINTF(outf, "    TEST EAX, EAX\n    JZ __lbl%u\n", lbl_end);
    if (n->child_count > 1) GEN_STMT(n->children[1]);
    AC_FPRINTF(outf, "    JMP __lbl%u\n__lbl%u:\n", lbl_start, lbl_end);
    ctx->loop_label_stack_top = old_top;
}

STATIC VOID GEN_DO_WHILE(PCNODE n) {
    U32 lbl_start = new_label(), lbl_end = new_label();
    U32 old_top = ctx->loop_label_stack_top;
    ctx->loop_label_stack[ctx->loop_label_stack_top++] = lbl_end;
    ctx->loop_label_stack[ctx->loop_label_stack_top++] = lbl_start;
    AC_FPRINTF(outf, "__lbl%u:\n", lbl_start);
    if (n->child_count > 0) GEN_STMT(n->children[0]);
    GEN_EXPR(n->children[1]);
    AC_FPRINTF(outf, "    TEST EAX, EAX\n    JNZ __lbl%u\n", lbl_start);
    AC_FPRINTF(outf, "__lbl%u:\n", lbl_end);
    ctx->loop_label_stack_top = old_top;
}

STATIC VOID GEN_FOR(PCNODE n) {
    /* init; cond (lbl_start); body; step; jmp lbl_start; lbl_end */
    U32 lbl_start = new_label(), lbl_end = new_label(), lbl_body = new_label();
    U32 old_top = ctx->loop_label_stack_top;
    ctx->loop_label_stack[ctx->loop_label_stack_top++] = lbl_end;
    ctx->loop_label_stack[ctx->loop_label_stack_top++] = lbl_start;

    if (n->child_count > 0 && n->children[0]->ntype != CNODE_INT_LIT)
        GEN_STMT(n->children[0]); /* init */
    AC_FPRINTF(outf, "__lbl%u:\n", lbl_start);
    if (n->child_count > 1) {
        GEN_EXPR(n->children[1]); /* cond */
        AC_FPRINTF(outf, "    TEST EAX, EAX\n    JZ __lbl%u\n", lbl_end);
    }
    /* body */
    if (n->child_count > 3) GEN_STMT(n->children[3]);
    /* step */
    AC_FPRINTF(outf, "__lbl%u:\n", lbl_body);
    if (n->child_count > 2) GEN_EXPR(n->children[2]);
    AC_FPRINTF(outf, "    JMP __lbl%u\n__lbl%u:\n", lbl_start, lbl_end);
    ctx->loop_label_stack_top = old_top;
}

STATIC VOID GEN_SWITCH(PCNODE n) {
    U32 lbl_end = new_label();
    GEN_EXPR(n->children[0]); /* switch value → EAX */
    for (U32 i = 1; i < n->child_count; i++) {
        PCNODE cs = n->children[i];
        if (cs->ntype == CNODE_CASE && cs->child_count > 0) {
            U32 lbl_next = new_label();
            GEN_EXPR(cs->children[0]); /* case value → EBX */
            emit("    MOV EBX, EAX");
            emit("    POP EAX");
            AC_FPRINTF(outf, "    CMP EAX, EBX\n    JNE __lbl%u\n", lbl_next);
            emit("    PUSH EAX");
            for (U32 j = 1; j < cs->child_count; j++) GEN_STMT(cs->children[j]);
            emit("    POP EAX");
            AC_FPRINTF(outf, "__lbl%u:\n", lbl_next);
        } else if (cs->ntype == CNODE_DEFAULT) {
            for (U32 j = 0; j < cs->child_count; j++) GEN_STMT(cs->children[j]);
        }
    }
    AC_FPRINTF(outf, "__lbl%u:\n", lbl_end);
}

STATIC VOID GEN_GOTO(PCNODE n) {
    AC_FPRINTF(outf, "    JMP %s\n", n->txt ? n->txt : (PU8)"_unknown");
}

STATIC VOID GEN_LABEL(PCNODE n) {
    AC_FPRINTF(outf, "%s:\n", n->txt ? n->txt : (PU8)"_unknown");
}

STATIC VOID GEN_RETURN(PCNODE n) {
    if (n->child_count > 0) GEN_EXPR(n->children[0]);
    if (local_offset < 0)
        AC_FPRINTF(outf, "    ADD ESP, %u\n", (U32)(-local_offset));
    emit("    POP EBP");
    emit("    RET");
}

STATIC VOID GEN_BREAK() {
    if (ctx->loop_label_stack_top >= 2)
        AC_FPRINTF(outf, "    JMP __lbl%u\n", ctx->loop_label_stack[ctx->loop_label_stack_top - 2]);
}

STATIC VOID GEN_CONTINUE() {
    if (ctx->loop_label_stack_top >= 2)
        AC_FPRINTF(outf, "    JMP __lbl%u\n", ctx->loop_label_stack[ctx->loop_label_stack_top - 1]);
}

STATIC VOID GEN_ASM_BLOCK(PCNODE n) {
    if (!n->txt) return;
    U8 *p = n->txt;
    while (*p) {
        if (!II_START(*p)) { AC_FPRINTF(outf, "%c", *p); p++; continue; }
        U8 name[128]; U32 ni = 0;
        while (*p && II_CONT(*p) && ni < 126) name[ni++] = *p++;
        name[ni] = '\0';
        II_UPPER(name, ni);
        SYMBOL *s = FIND_SYM(name);
        if (s && !s->is_global) {
            if (*p == '.') {
                p++;
                U8 fname[64]; U32 fi = 0;
                while (*p && II_CONT(*p) && fi < 62) fname[fi++] = *p++;
                fname[fi] = '\0';
                II_UPPER(fname, fi);
                SYMBOL *ts = FIND_SYM(s->type.name);
                U32 foff = 0;
                if (ts) {
                    for (U32 j = 0; j < ts->field_count; j++) {
                        if (ts->fields[j].name && AC_STRCMP(ts->fields[j].name, fname) == 0)
                            { foff = ts->fields[j].offset; break; }
                    }
                }
                U32 addr = (U32)((I32)s->offset + (I32)foff);
                if ((I32)addr == 0)
                    AC_FPRINTF(outf, "[EBP]");
                else if ((I32)addr > 0)
                    AC_FPRINTF(outf, "[EBP+%u]", addr);
                else
                    AC_FPRINTF(outf, "[EBP-%u]", (U32)(-(I32)addr));
            } else {
                if ((I32)s->offset == 0)
                    AC_FPRINTF(outf, "[EBP]");
                else if ((I32)s->offset > 0)
                    AC_FPRINTF(outf, "[EBP+%u]", s->offset);
                else
                    AC_FPRINTF(outf, "[EBP-%u]", (U32)(-(I32)s->offset));
            }
        } else if (s && s->is_global) {
            AC_FPRINTF(outf, "[%s]", s->name);
        } else {
            AC_FPRINTF(outf, "%s", name);
        }
    }
    AC_FPRINTF(outf, "\n");
}

STATIC VOID GEN_DEBUG_LINE(PCNODE n) {
    if (!ctx->debug || !ctx->src_lines || !n) return;
    U32 line = n->line;
    if (line == 0 || line > ctx->src_line_count) return;
    PU8 txt = ctx->src_lines[line - 1];
    if (!txt) return;
    while (*txt == ' ' || *txt == '\t') txt++;
    if (*txt) AC_FPRINTF(outf, "; %s\n", txt);
}

STATIC VOID GEN_STMT(PCNODE n) {
    if (!n) return;
    GEN_DEBUG_LINE(n);
    switch (n->ntype) {
        case CNODE_BLOCK:    GEN_BLOCK(n); break;
        case CNODE_IF:       GEN_IF(n); break;
        case CNODE_WHILE:    GEN_WHILE(n); break;
        case CNODE_DO_WHILE: GEN_DO_WHILE(n); break;
        case CNODE_FOR:      GEN_FOR(n); break;
        case CNODE_SWITCH:   GEN_SWITCH(n); break;
        case CNODE_GOTO:     GEN_GOTO(n); break;
        case CNODE_LABEL:    GEN_LABEL(n); break;
        case CNODE_RETURN:   GEN_RETURN(n); break;
        case CNODE_BREAK:    GEN_BREAK(); break;
        case CNODE_CONTINUE: GEN_CONTINUE(); break;
        case CNODE_ASM_BLOCK: GEN_ASM_BLOCK(n); break;
        case CNODE_VAR_DECL: {
            /* Array decls: children[0] is size (INT_LIT), not an initializer.
             * Scalar init / array init: first non-size child is the value. */
            SYMBOL *s = FIND_SYM(n->txt);
            PCNODE init = NULLPTR;
            if (s && s->array_size > 0) {
                if (n->child_count > 1) init = n->children[1];
            } else if (n->child_count > 0) {
                init = n->children[0];
            }
            if (init) {
                GEN_EXPR(init);
                U32 es = s ? COMP_TYPE_SIZE(s->type) : 4;
                if (s && s->is_global) {
                    if (es == 1)      AC_FPRINTF(outf, "    MOV [%s], AL\n", s->name);
                    else if (es == 2) AC_FPRINTF(outf, "    MOV [%s], AX\n", s->name);
                    else              AC_FPRINTF(outf, "    MOV [%s], EAX\n", s->name);
                } else if (s && (I32)s->offset > 0) {
                    if (es == 1)      AC_FPRINTF(outf, "    MOV [EBP+%u], AL\n", s->offset);
                    else if (es == 2) AC_FPRINTF(outf, "    MOV [EBP+%u], AX\n", s->offset);
                    else              AC_FPRINTF(outf, "    MOV [EBP+%u], EAX\n", s->offset);
                } else if (s) {
                    U32 off = (U32)(-(I32)s->offset);
                    if (es == 1)      AC_FPRINTF(outf, "    MOV [EBP-%u], AL\n", off);
                    else if (es == 2) AC_FPRINTF(outf, "    MOV [EBP-%u], AX\n", off);
                    else              AC_FPRINTF(outf, "    MOV [EBP-%u], EAX\n", off);
                }
            }
            break;
        }
        case CNODE_EXPR_STMT:
            if (n->child_count > 0) GEN_EXPR(n->children[0]);
            break;
        default: break;
    }
}

/* ── Variable offset assignment ───────────────────────────────────────────── */
STATIC VOID ASSIGN_LOCAL_OFFSETS(PCNODE n) {
    if (!n) return;
    if (n->ntype == CNODE_VAR_DECL && n->txt) {
        SYMBOL *s = FIND_SYM(n->txt);
        if (s && !s->is_global) {
            U32 es = COMP_TYPE_SIZE(n->dtype);
            if (es == 0) es = 4;
            U32 count = (s->array_size > 0) ? s->array_size : 1;
            U32 sz = es * count;
            local_offset -= (I32)sz;
            s->offset = (U32)local_offset;
        }
    }
    for (U32 i = 0; i < n->child_count; i++)
        ASSIGN_LOCAL_OFFSETS(n->children[i]);
}

/* ── Emit string literals in .rodata ──────────────────────────────────────── */
STATIC VOID EMIT_RODATA_STRINGS() {
    for (U32 i = 0; i < ctx->rodata_string_count; i++) {
        RODATA_STR *rs = &ctx->rodata_strings[i];
        if (rs->node && rs->node->txt) {
            AC_FPRINTF(outf, "_str%u DB \"", rs->label);
            /* Write the string content with escape handling */
            PU8 txt = rs->node->txt;
            while (*txt) {
                U8 c = (U8)*txt;
                switch (c) {
                    case '\n': AC_FPRINTF(outf, "\\n"); break;
                    case '\r': AC_FPRINTF(outf, "\\r"); break;
                    case '\t': AC_FPRINTF(outf, "\\t"); break;
                    case '\\': AC_FPRINTF(outf, "\\\\"); break;
                    case '"':  AC_FPRINTF(outf, "\\\""); break;
                    default:   AC_FPRINTF(outf, "%c", c); break;
                }
                txt++;
            }
            AC_FPRINTF(outf, "\", 0\n");
        }
    }
}

/* ── Top-level codegen ────────────────────────────────────────────────────── */

BOOL COMP_GEN(PCNODE root, PCOMP_CTX c) {
    if (!root || !c) return FALSE;
    ctx = c; sym = &c->symtab;

    PU8 outfile = c->out_asm;
    if (!outfile) outfile = (PU8)"/tmp/out.AS";
    if (AC_FILE_EXISTS(outfile)) AC_FILE_DELETE(outfile);
    AC_FILE_CREATE(outfile);
    outf = AC_FOPEN(outfile, MODE_FA);
    if (!outf) { AC_PRINTF("[GEN] Failed to open %s\n", outfile); return FALSE; }

    ASTRAC_ARGS *cfg = GET_ARGS();
    BOOL bits16 = (cfg && cfg->dsm_bits == 16);
    BOOL bootloader = (cfg && cfg->org > 0 && bits16);
    emit("; ASM output generated by ASTRAC Compiler. DO NOT EDIT.\n");
    if (cfg && cfg->org > 0)
        AC_FPRINTF(outf, ".org 0x%X\n", cfg->org);
    emit((bits16 ? ".use16" : ".use32"));
    emit(".code");
    
    // Emit entry point jump.  Function labels are emitted as `_<NAME>`
    // where NAME is the upper-cased symbol, so the jump target must match.
    {
        U8 buf[64];
        PU8 ep = (cfg && cfg->entry_point) ? cfg->entry_point : (PU8)"_start";
        U32 i;
        AC_STRNCPY(buf, ep, 60);
        buf[60] = '\0';
        for (i = 0; buf[i]; i++)
            if (buf[i] >= 'a' && buf[i] <= 'z') buf[i] -= 32;
        AC_FPRINTF(outf, "jmp _%s\n", buf);
    }

    /* Emit functions */
    for (U32 i = 0; i < root->child_count; i++) {
        PCNODE n = root->children[i];
        if (n->ntype != CNODE_FUNC_DECL) continue;
        if (!n->txt) continue;
        SYMBOL *fs = FIND_SYM(n->txt);
        if (!fs || !fs->is_defined) continue;
        /* Skip forward declarations (no body) */
        BOOL has_body = FALSE;
        for (U32 j = 0; j < n->child_count; j++)
            if (n->children[j]->ntype != CNODE_PARAM) { has_body = TRUE; break; }
        if (!has_body) continue;

        if (!bootloader) {
            GEN_DEBUG_LINE(n);
            AC_FPRINTF(outf, "\n_%s:\n", fs->name);
            emit("    PUSH EBP");
            emit("    MOV EBP, ESP");
        }

        /* Assign positive EBP offsets to parameters (EBP+8, EBP+12, ...) */
        {
            U32 param_off = 8;
            for (U32 j = 0; j < n->child_count; j++) {
                PCNODE child = n->children[j];
                if (child->ntype == CNODE_PARAM && child->txt) {
                    SYMBOL *ps = FIND_SYM(child->txt);
                    if (ps) { ps->offset = param_off; param_off += 4; }
                }
            }
        }

        /* Assign stack offsets to local variables */
        local_offset = 0;
        ASSIGN_LOCAL_OFFSETS(n);
        /* Make space for locals */
        if (!bootloader && local_offset < 0)
            AC_FPRINTF(outf, "    SUB ESP, %u\n", (U32)(-local_offset));

        /* Generate body */
        for (U32 j = 0; j < n->child_count; j++) {
            PCNODE child = n->children[j];
            if (child->ntype != CNODE_PARAM) GEN_STMT(child);
        }

        /* Default return */
        if (!bootloader) {
            AC_FPRINTF(outf, "__%s_ret:\n", fs->name);
            if (local_offset < 0)
                AC_FPRINTF(outf, "    ADD ESP, %u\n", (U32)(-local_offset));
            emit("    POP EBP");
            emit("    RET");
        }
    }

    /* Emit string literals in .rodata (skip in bootloader mode) */
    if (!bootloader && ctx->rodata_string_count > 0) {
        emit("\n.rodata");
        EMIT_RODATA_STRINGS();
    }

    /* Emit global variables in .data (skip in bootloader mode) */
    if (!bootloader) {
        BOOL has_globals = FALSE;
        for (U32 i = 0; i < sym->count; i++) {
            SYMBOL *s = &sym->entries[i];
            if (s->kind == SYM_VARIABLE && s->is_global && !s->is_defined) {
                if (!has_globals) { emit("\n.data"); has_globals = TRUE; }
                U32 count = (s->array_size > 0) ? s->array_size : 1;
                U32 elem_size = COMP_TYPE_SIZE(s->type);
                PU8 der = (elem_size == 1) ? "DB" : (elem_size == 2) ? "DW" : "DD";
                if (count == 1)
                    AC_FPRINTF(outf, "%s %s 0\n", s->name, der);
                else
                    AC_FPRINTF(outf, "%s:\n.times %u %s 0\n", s->name, count, der);
            }
        }
    }

    AC_FCLOSE(outf);
    return TRUE;
}
