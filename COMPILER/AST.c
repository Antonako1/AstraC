/*
 * COMPILER/AST.c — Compiler AST node helpers (skeleton).
 *
 * All nodes are heap-allocated. Children are stored in a dynamically-grown
 * array inside each node (CNODE.children).
 * TODO: implement.
 */
#include "COMPILER.h"

U32 COMP_TYPE_SIZE(COMP_TYPE t) {
    /* TODO */
    (void)t;
    return 0;
}

PCNODE CNODE_NEW(CNODE_TYPE ntype, U32 line, U32 col) {
    /* TODO */
    (void)ntype; (void)line; (void)col;
    return NULLPTR;
}

VOID CNODE_ADD_CHILD(PCNODE parent, PCNODE child) {
    /* TODO */
    (void)parent; (void)child;
}

PCNODE CNODE_INT(U32 val, U32 line, U32 col) {
    /* TODO */
    (void)val; (void)line; (void)col;
    return NULLPTR;
}

PCNODE CNODE_FLOAT(F32 val, U32 line, U32 col) {
    /* TODO */
    (void)val; (void)line; (void)col;
    return NULLPTR;
}

PCNODE CNODE_STR(PU8 txt, U32 line, U32 col) {
    /* TODO */
    (void)txt; (void)line; (void)col;
    return NULLPTR;
}

PCNODE CNODE_IDENT_NODE(PU8 name, U32 line, U32 col) {
    /* TODO */
    (void)name; (void)line; (void)col;
    return NULLPTR;
}

VOID DESTROY_CNODE(PCNODE node) {
    /* TODO */
    (void)node;
}

VOID DESTROY_CNODE_TREE(PCNODE root) {
    /* TODO */
    (void)root;
}
