/*
 * COMPILER/AST.c — Compiler AST node helpers.
 *
 * All nodes are heap-allocated. Children are stored in a dynamically-grown
 * array inside each node (CNODE.children).
 */
#include "COMPILER.h"

U32 COMP_TYPE_SIZE(COMP_TYPE t) {
    static const U8 base_sizes[] = {
        [CTYPE_NONE]    = 0,
        [CTYPE_U8]      = 1, [CTYPE_I8]     = 1,
        [CTYPE_U16]     = 2, [CTYPE_I16]    = 2,
        [CTYPE_U32]     = 4, [CTYPE_I32]    = 4,
        [CTYPE_F32]     = 4,
        [CTYPE_U0]      = 0,
        [CTYPE_PU8]     = 4, [CTYPE_PU16]   = 4, [CTYPE_PU32]   = 4,
        [CTYPE_PPU8]    = 4, [CTYPE_PPU16]  = 4, [CTYPE_PPU32]  = 4,
        [CTYPE_PI8]     = 4, [CTYPE_PI16]   = 4, [CTYPE_PI32]   = 4,
        [CTYPE_PPI8]    = 4, [CTYPE_PPI16]  = 4, [CTYPE_PPI32]  = 4,
        [CTYPE_BOOL]    = 4,
        [CTYPE_VOIDPTR] = 4,
        [CTYPE_STRUCT]  = 0, [CTYPE_ENUM]   = 4, [CTYPE_UNION]   = 0,
    };
    U32 sz = (t.base < sizeof(base_sizes)) ? base_sizes[t.base] : 0;
    while (t.ptr_depth-- > 0) sz = 4;
    return sz;
}

STATIC COMP_TYPE COMP_BASE_TO_TYPE(COMP_BASE_TYPE base, U32 ptr_depth, PU8 name) {
    COMP_TYPE t;
    t.base      = base;
    t.ptr_depth = (U8)ptr_depth;
    t.name      = name;
    return t;
}

PCNODE CNODE_NEW(CNODE_TYPE ntype, U32 line, U32 col) {
    PCNODE n = (PCNODE)AC_MAlloc(sizeof(CNODE));
    if (!n) return NULLPTR;
    AC_MEMZERO(n, sizeof(CNODE));
    n->ntype       = ntype;
    n->line        = line;
    n->col         = col;
    n->dtype.base  = CTYPE_NONE;
    n->children    = NULLPTR;
    n->child_count = 0;
    n->child_cap   = 0;
    return n;
}

VOID CNODE_ADD_CHILD(PCNODE parent, PCNODE child) {
    if (!parent || !child) return;
    if (parent->child_count >= parent->child_cap) {
        U32 new_cap = parent->child_cap ? parent->child_cap * 2 : 8;
        PCNODE *n = (PCNODE *)AC_ReAlloc(parent->children, new_cap * sizeof(PCNODE));
        if (!n) return;
        parent->children  = n;
        parent->child_cap = new_cap;
    }
    parent->children[parent->child_count++] = child;
}

PCNODE CNODE_INT(U32 val, U32 line, U32 col) {
    PCNODE n = CNODE_NEW(CNODE_INT_LIT, line, col);
    n->ival  = val;
    n->dtype = COMP_BASE_TO_TYPE(CTYPE_U32, 0, NULLPTR);
    return n;
}

PCNODE CNODE_FLOAT(F32 val, U32 line, U32 col) {
    PCNODE n = CNODE_NEW(CNODE_FLOAT_LIT, line, col);
    n->fval  = val;
    n->dtype = COMP_BASE_TO_TYPE(CTYPE_F32, 0, NULLPTR);
    return n;
}

PCNODE CNODE_STR(PU8 txt, U32 line, U32 col) {
    PCNODE n  = CNODE_NEW(CNODE_STR_LIT, line, col);
    n->txt    = AC_STRDUP(txt);
    n->dtype  = COMP_BASE_TO_TYPE(CTYPE_PU8, 0, NULLPTR);
    n->ival   = 0;
    return n;
}

PCNODE CNODE_IDENT_NODE(PU8 name, U32 line, U32 col) {
    PCNODE n = CNODE_NEW(CNODE_IDENT, line, col);
    n->txt   = AC_STRDUP(name);
    return n;
}

VOID DESTROY_CNODE(PCNODE n) {
    if (!n) return;
    for (U32 i = 0; i < n->child_count; i++)
        DESTROY_CNODE(n->children[i]);
    if (n->children) AC_MFree(n->children);
    if (n->txt)      AC_MFree(n->txt);
    AC_MFree(n);
}

VOID DESTROY_CNODE_TREE(PCNODE root) {
    DESTROY_CNODE(root);
}
