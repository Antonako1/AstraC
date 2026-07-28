/**
 * COMPILER/COMPILER.h — AC Language compiler types and interface.
 *
 * Included by the compiler pipeline stages and by ASSEMBLER.h
 * (the assembler needs COMP_* types for the full-build pipeline).
 */
#ifndef COMPILER_H
#define COMPILER_H

#include "../AstraC.h"

/* ════════════════════════════════════════════════════════════════════════════
 *  TOKEN TYPES
 *  The lexer normalises all identifiers and keywords to UPPER CASE.
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _COMP_TOK_TYPE {
    CTOK_EOF = 0,
    CTOK_ERROR,

    /* Literals */
    CTOK_INT_LIT,
    CTOK_FLOAT_LIT,
    CTOK_STR_LIT,
    CTOK_CHAR_LIT,

    CTOK_IDENT,

    /* Primitive types */
    CTOK_KW_U8,    CTOK_KW_I8,
    CTOK_KW_U16,   CTOK_KW_I16,
    CTOK_KW_U32,   CTOK_KW_I32,

    CTOK_KW_PU8,   CTOK_KW_PU16,  CTOK_KW_PU32,
    CTOK_KW_PPU8,  CTOK_KW_PPU16, CTOK_KW_PPU32,
    CTOK_KW_PI8,   CTOK_KW_PI16,  CTOK_KW_PI32,
    CTOK_KW_PPI8,  CTOK_KW_PPI16, CTOK_KW_PPI32,

    CTOK_KW_F32,
    CTOK_KW_U0,
    CTOK_KW_VOID,
    CTOK_KW_BOOL,
    CTOK_KW_TRUE,
    CTOK_KW_FALSE,
    CTOK_KW_NULLPTR,
    CTOK_KW_VOIDPTR,

    /* Control flow */
    CTOK_KW_IF,      CTOK_KW_ELSE,
    CTOK_KW_FOR,     CTOK_KW_WHILE,   CTOK_KW_DO,
    CTOK_KW_RETURN,  CTOK_KW_BREAK,   CTOK_KW_CONTINUE,
    CTOK_KW_SWITCH,  CTOK_KW_CASE,    CTOK_KW_DEFAULT,
    CTOK_KW_GOTO,

    /* Declarations */
    CTOK_KW_STRUCT,  CTOK_KW_UNION,
    CTOK_KW_ENUM,    CTOK_KW_SIZEOF,
    CTOK_KW_TYPEDEF, CTOK_KW_STATIC, CTOK_KW_LOCAL,

    CTOK_KW_ASM,

    /* Arithmetic */
    CTOK_PLUS, CTOK_MINUS, CTOK_STAR, CTOK_SLASH, CTOK_PERCENT,
    CTOK_PLUSPLUS, CTOK_MINUSMINUS,

    /* Relational */
    CTOK_EQ, CTOK_NEQ, CTOK_LT, CTOK_GT, CTOK_LE, CTOK_GE,

    /* Logical */
    CTOK_AND, CTOK_OR, CTOK_NOT,

    /* Bitwise */
    CTOK_AMP, CTOK_PIPE, CTOK_CARET, CTOK_TILDE, CTOK_SHL, CTOK_SHR,

    /* Assignment */
    CTOK_ASSIGN,
    CTOK_PLUS_ASSIGN, CTOK_MINUS_ASSIGN,
    CTOK_STAR_ASSIGN, CTOK_SLASH_ASSIGN, CTOK_PCT_ASSIGN,
    CTOK_AMP_ASSIGN,  CTOK_PIPE_ASSIGN,  CTOK_CARET_ASSIGN,
    CTOK_SHL_ASSIGN,  CTOK_SHR_ASSIGN,

    /* Member access */
    CTOK_ARROW, CTOK_DOT,

    /* Misc */
    CTOK_QUESTION, CTOK_COLON,

    /* Delimiters */
    CTOK_LPAREN, CTOK_RPAREN,
    CTOK_LBRACE, CTOK_RBRACE,
    CTOK_LBRACKET, CTOK_RBRACKET,
    CTOK_SEMICOLON, CTOK_COMMA,
} COMP_TOK_TYPE;

/* ════════════════════════════════════════════════════════════════════════════
 *  TOKEN
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct _COMP_TOK {
    COMP_TOK_TYPE type;
    PU8  txt;
    U32  line;
    U32  col;
    union {
        U32 ival;
        F32 fval;
    };
} COMP_TOK, *PCOMP_TOK;

typedef struct {
    PCOMP_TOK *toks;
    U32 len;
    U32 cap;
} COMP_TOK_ARRAY, *PCOMP_TOK_ARRAY;

/* ════════════════════════════════════════════════════════════════════════════
 *  TYPE SYSTEM
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _COMP_BASE_TYPE {
    CTYPE_NONE = 0,
    CTYPE_U8,  CTYPE_I8,
    CTYPE_U16, CTYPE_I16,
    CTYPE_U32, CTYPE_I32,
    CTYPE_F32,
    CTYPE_U0,
    CTYPE_PU8,  CTYPE_PU16,  CTYPE_PU32,
    CTYPE_PPU8, CTYPE_PPU16, CTYPE_PPU32,
    CTYPE_PI8,  CTYPE_PI16,  CTYPE_PI32,
    CTYPE_PPI8, CTYPE_PPI16, CTYPE_PPI32,
    CTYPE_BOOL,
    CTYPE_VOIDPTR,
    CTYPE_STRUCT,
    CTYPE_ENUM,
    CTYPE_UNION,
} COMP_BASE_TYPE;

typedef struct _COMP_TYPE {
    COMP_BASE_TYPE base;
    U8  ptr_depth;
    PU8 name;       /* struct/union/enum name, or NULL */
} COMP_TYPE;

U32 COMP_TYPE_SIZE(COMP_TYPE t);

/* ════════════════════════════════════════════════════════════════════════════
 *  SYMBOL TABLE
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _SYM_KIND {
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_STRUCT,
    SYM_UNION,
    SYM_ENUM,
    SYM_TYPEDEF,
} SYM_KIND;

typedef struct _TYPE_FIELD {
    PU8 name;
    COMP_TYPE type;
    U32 offset;
    U32 size;
} TYPE_FIELD;

typedef struct _SYMBOL {
    PU8 name;
    SYM_KIND kind;
    COMP_TYPE type;           /* for variables/functions/typedefs */
    COMP_TYPE ret_type;       /* for functions */
    U32  offset;              /* stack offset for locals, 0 for globals */
    U32  ival;                /* enum constant value */
    BOOL is_global;           /* TRUE = .data/.rodata, FALSE = stack */
    BOOL is_defined;          /* function has body (not just forward decl) */
    BOOL is_variadic;
    BOOL is_union;            /* for struct/unions */

    /* Function params */
    U32   param_count;
    COMP_TYPE param_types[16];
    PU8       param_names[16];

    /* Struct/union fields */
    TYPE_FIELD fields[64];
    U32  field_count;
    U32  total_size;

    /* Enum values: enum constant name -> U32 value stored via type */
} SYMBOL;

typedef struct _SYM_TABLE {
    SYMBOL entries[512];
    U32    count;
} SYM_TABLE;

typedef struct {
    COMP_TOK_TYPE key;
    PU8           kw_str;
} COMP_KW_MAP;

/* ════════════════════════════════════════════════════════════════════════════
 *  AST NODE TYPES
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _CNODE_TYPE {
    CNODE_PROGRAM,
    CNODE_FUNC_DECL,  CNODE_VAR_DECL,  CNODE_PARAM,
    CNODE_STRUCT_DECL, CNODE_UNION_DECL, CNODE_ENUM_DECL, CNODE_ENUM_FIELD,
    CNODE_BLOCK,
    CNODE_RETURN,
    CNODE_IF,   CNODE_FOR,  CNODE_WHILE, CNODE_DO_WHILE,
    CNODE_BREAK, CNODE_CONTINUE,
    CNODE_SWITCH, CNODE_CASE, CNODE_DEFAULT,
    CNODE_GOTO,   CNODE_LABEL,
    CNODE_EXPR_STMT, CNODE_ASM_BLOCK,
    CNODE_BINARY, CNODE_UNARY, CNODE_POSTFIX, CNODE_TERNARY,
    CNODE_CALL,   CNODE_INDEX, CNODE_MEMBER,  CNODE_ARROW_EXPR,
    CNODE_ASSIGN, CNODE_CAST,
    CNODE_SIZEOF_TYPE, CNODE_SIZEOF_EXPR,
    CNODE_ADDR, CNODE_DEREF,
    CNODE_INT_LIT, CNODE_FLOAT_LIT, CNODE_STR_LIT, CNODE_CHAR_LIT,
    CNODE_IDENT,   CNODE_NULLPTR,   CNODE_TRUE_LIT, CNODE_FALSE_LIT,
} CNODE_TYPE;

/* ════════════════════════════════════════════════════════════════════════════
 *  AST NODE
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct _CNODE {
    CNODE_TYPE     ntype;
    COMP_TYPE      dtype;
    PU8            txt;
    U32            line;
    U32            col;
    COMP_TOK_TYPE  op;
    union {
        U32 ival;
        F32 fval;
    };
    struct _CNODE **children;
    U32  child_count;
    U32  child_cap;
} CNODE, *PCNODE;

typedef struct {
    PCNODE *nodes;
    U32 len;
    U32 cap;
} CNODE_ARRAY, *PCNODE_ARRAY;

typedef struct {
    U32    label;
    PCNODE node;
} RODATA_STR;

/* ════════════════════════════════════════════════════════════════════════════
 *  COMPILER CONTEXT
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct _COMP_CTX {
    PU8 tmp_src;
    PU8 out_asm;

    U32 label_counter;
    U32 loop_label_stack[32];
    U32 loop_label_stack_top;

    RODATA_STR rodata_strings[256];
    U32        rodata_string_count;

    SYM_TABLE  symtab;       /* symbol table for parser/verifier/codegen */
    BOOL       in_func;      /* inside a function body */
    BOOL       verbose;      /* verbose output */
    PCNODE     cur_func;     /* current function node */

    U32 errors;
    U32 warnings;
} COMP_CTX, *PCOMP_CTX;

/* ════════════════════════════════════════════════════════════════════════════
 *  FUNCTION DECLARATIONS
 * ════════════════════════════════════════════════════════════════════════════ */

U32 START_COMPILER();

/* Lexer */
PCOMP_TOK_ARRAY COMP_LEX(PCOMP_CTX ctx);
VOID            DESTROY_COMP_TOK_ARRAY(PCOMP_TOK_ARRAY toks);

/* Parser */
PCNODE COMP_PARSE(PCOMP_TOK_ARRAY toks, PCOMP_CTX ctx);
VOID   DESTROY_CNODE(PCNODE node);
VOID   DESTROY_CNODE_TREE(PCNODE root);

/* AST helpers */
PCNODE CNODE_NEW(CNODE_TYPE ntype, U32 line, U32 col);
VOID   CNODE_ADD_CHILD(PCNODE parent, PCNODE child);
PCNODE CNODE_INT(U32 val, U32 line, U32 col);
PCNODE CNODE_FLOAT(F32 val, U32 line, U32 col);
PCNODE CNODE_STR(PU8 txt, U32 line, U32 col);
PCNODE CNODE_IDENT_NODE(PU8 name, U32 line, U32 col);

/* Verification */
BOOL COMP_VERIFY(PCNODE root, PCOMP_CTX ctx);

/* Code generation */
BOOL COMP_GEN(PCNODE root, PCOMP_CTX ctx);

#endif /* COMPILER_H */
