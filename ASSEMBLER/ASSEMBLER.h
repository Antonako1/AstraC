/**
 * ASSEMBLER/ASSEMBLER.h — x86 assembler types and interface.
 */
#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "../AstraC.h"
#include "../COMPILER/COMPILER.h"

/* ════════════════════════════════════════════════════════════════════════════
 *  PREPROCESSOR OUTPUT
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    PU8 tmp_files[MAX_FILES];
    U32 tmp_file_count;
} ASM_INFO, *PASM_INFO;


/* ════════════════════════════════════════════════════════════════════════════
 *  REGISTERS
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _ASM_REGS {
    REG_NONE = -1,

    REG_EAX, REG_EBX, REG_ECX, REG_EDX,
    REG_ESI, REG_EDI, REG_EBP, REG_ESP,

    REG_AX, REG_BX, REG_CX, REG_DX,
    REG_SI, REG_DI, REG_BP, REG_SP,

    REG_AL, REG_AH, REG_BL, REG_BH,
    REG_CL, REG_CH, REG_DL, REG_DH,

    REG_IP,  REG_EIP,

    REG_CS, REG_DS, REG_ES, REG_FS, REG_GS, REG_SS,

    REG_CR0, REG_CR2, REG_CR3, REG_CR4,

    REG_DR0, REG_DR1, REG_DR2, REG_DR3, REG_DR6, REG_DR7,

    REG_ST0, REG_ST1, REG_ST2, REG_ST3,
    REG_ST4, REG_ST5, REG_ST6, REG_ST7,

    REG_MM0, REG_MM1, REG_MM2, REG_MM3,
    REG_MM4, REG_MM5, REG_MM6, REG_MM7,

    REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3,
    REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7,
} ASM_REGS;


/* ════════════════════════════════════════════════════════════════════════════
 *  SYMBOLS
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _ASM_SYMBOLS {
    SYM_NONE = 0,
    SYM_COMMA,      SYM_COLON,      SYM_SEMICOLON,
    SYM_LBRACKET,   SYM_RBRACKET,
    SYM_LPAREN,     SYM_RPAREN,
    SYM_PLUS,       SYM_MINUS,      SYM_ASTERISK,
    SYM_SLASH,      SYM_PERCENT,
    SYM_AND,        SYM_OR,         SYM_XOR,        SYM_TILDE,
    SYM_SHL,        SYM_SHR,
    SYM_EQUALS,
    SYM_DOT,        SYM_AT,
    SYM_DOLLAR,     SYM_DOLLAR_DOLLAR,
    SYM_AMOUNT
} ASM_SYMBOLS;


/* ════════════════════════════════════════════════════════════════════════════
 *  DATA TYPES
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _ASM_VAR_TYPE {
    TYPE_NONE,
    TYPE_BYTE,
    TYPE_WORD,
    TYPE_DWORD,
    TYPE_FLOAT,
    TYPE_PTR,
    TYPE_NEAR,
    TYPE_FAR,
    TYPE_AMOUNT,
} ASM_VAR_TYPE;


/* ════════════════════════════════════════════════════════════════════════════
 *  TOKEN TYPES
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _ASM_TOKEN_TYPE {
    TOK_NONE,
    TOK_EOL,
    TOK_EOF,
    TOK_LABEL,
    TOK_LOCAL_LABEL,
    TOK_MNEMONIC,
    TOK_REGISTER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_SYMBOL,
    TOK_IDENT_VAR,
    TOK_IDENTIFIER,
    TOK_DIRECTIVE,
    TOK_AMOUNT,
} ASM_TOKEN_TYPE;


/* ════════════════════════════════════════════════════════════════════════════
 *  DIRECTIVES
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _ASM_DIRECTIVE {
    DIR_NONE,
    DIR_DATA,
    DIR_RODATA,
    DIR_CODE,
    DIR_CODE_TYPE_32,
    DIR_CODE_TYPE_16,
    DIR_ORG,
    DIR_TIMES,
    DIR_AMOUNT,
} ASM_DIRECTIVE;


/* ════════════════════════════════════════════════════════════════════════════
 *  TOKEN
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    ASM_TOKEN_TYPE type;
    PU8  txt;
    U32  line;
    U32  col;
    union {
        U32           num_val;
        ASM_REGS      reg;
        U32           mnemonic;
        ASM_SYMBOLS   symbol;
        ASM_DIRECTIVE directive;
        ASM_VAR_TYPE  var_type;
    };
} ASM_TOK, *PASM_TOK;

#define MAX_TOKENS 65536

typedef struct {
    U32      len;
    PASM_TOK toks[MAX_TOKENS];
} ASM_TOK_ARRAY;


/* ════════════════════════════════════════════════════════════════════════════
 *  KEYWORD TABLE
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    PU8 value;
    U32 enum_val;
} KEYWORD;

const KEYWORD *get_registers();
const KEYWORD *get_directives();
const KEYWORD *get_symbols();
const KEYWORD *get_variable_types();
const char    *TOKEN_TYPE_STR(ASM_TOKEN_TYPE t);


/* ════════════════════════════════════════════════════════════════════════════
 *  DATA VARIABLE
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct _ASM_VAR {
    ASM_VAR_TYPE var_type;
    PU8  name;
    PU8  raw_value;
    BOOL is_list;
    U32  list_len;
} ASM_VAR, *PASM_VAR;


/* ════════════════════════════════════════════════════════════════════════════
 *  AST NODE TYPES
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum {
    NODE_INVALID = 0,
    NODE_INSTRUCTION,
    NODE_LABEL,
    NODE_LOCAL_LABEL,
    NODE_RAW_NUM,
    NODE_DATA_VAR,
    NODE_ORG,
    NODE_TIMES,
    NODE_SECTION,
} ASM_NODE_TYPE;


/* ════════════════════════════════════════════════════════════════════════════
 *  ENCODING / OPERAND ENUMS
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum { ENC_DIRECT, ENC_REG_OPCODE, ENC_MODRM, ENC_IMM } ASM_ENCODING_TYPE;
typedef enum { OP_NONE, OP_REG, OP_MEM, OP_IMM, OP_SEG, OP_PTR, OP_FAR, OP_MOFFS } ASM_OPERAND_TYPE;
typedef enum { SZ_NONE = 0, SZ_8BIT = 8, SZ_16BIT = 16, SZ_32BIT = 32 } ASM_OPERAND_SIZE;
typedef enum { OPN_NONE = 0, OPN_ONE = 1, OPN_TWO = 2, OPN_THREE = 3, OPN_FOUR = 4 } ASM_OPERAND_COUNT;
typedef enum {
    MODRM_NONE = -1,
    MODRM_0, MODRM_1, MODRM_2, MODRM_3,
    MODRM_4, MODRM_5, MODRM_6, MODRM_7,
} ASM_MODRM_EXTENSION;
typedef enum { PF_NONE = 0x00, PF_66 = 0x66, PF_F2 = 0xF2, PF_F3 = 0xF3 } ASM_PREFIX;
typedef enum { PFX_NONE = 0, PFX_0F = 1 } ASM_OPCODE_PREFIX;
typedef enum { PROC_ANY, PROC_8086, PROC_80186, PROC_80286, PROC_80386, PROC_PENTIUM } ASM_PROCESSOR;
typedef enum { ST_DOCUMENTED, ST_MARGINALLY, ST_UNDOCUMENTED } ASM_DOC_STATUS;
typedef enum { MODE_REAL, MODE_PROTECTED, MODE_V86, MODE_ANY = MODE_REAL | MODE_PROTECTED | MODE_V86 } ASM_CPU_MODE;
typedef enum { MEM_NONE, MEM_READ, MEM_WRITE, MEM_RW } ASM_MEMORY_ACCESS;
typedef enum { RL_NONE, RL_REL8, RL_REL16, RL_REL32 } ASM_RELTYPE;
typedef enum { X_NONE, X_LOCK, X_FPU_PUSH, X_FPU_POP } ASM_LOCK_FPU_TYPE;
typedef enum { EX_NONE, EX_IMPLICIT, EX_PREFIX_0F } ASM_OPCODE_EXTENSION;
typedef enum {
    FLG_NONE = 0,
    FLG_O = 1 << 0, FLG_S = 1 << 1, FLG_Z = 1 << 2,
    FLG_A = 1 << 3, FLG_P = 1 << 4, FLG_C = 1 << 5,
} ASM_FLAGS;

#define FLG_ARITH      (FLG_O | FLG_S | FLG_Z | FLG_A | FLG_P | FLG_C)
#define FLG_ALL        FLG_ARITH
#define FLG_LOGIC      (FLG_S | FLG_Z | FLG_P)
#define FLG_ARITH_NO_C (FLG_O | FLG_S | FLG_Z | FLG_A | FLG_P)

/* ── Operand-type shorthand macros ─────────────────────────────────────── */
#define OPS_NONE           { OP_NONE,  OP_NONE, OP_NONE, OP_NONE }
#define OPS_PTR            { OP_PTR,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_FAR16          { OP_FAR,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_REG            { OP_REG,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_SEG            { OP_SEG,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_RM16           { OP_MEM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_RM32           { OP_MEM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_REL8           { OP_IMM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_REL16          { OP_IMM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_REL32          { OP_IMM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_RM8_R8         { OP_MEM,   OP_REG,  OP_NONE, OP_NONE }
#define OPS_RM16_R16       { OP_MEM,   OP_REG,  OP_NONE, OP_NONE }
#define OPS_R8_RM8         { OP_REG,   OP_MEM,  OP_NONE, OP_NONE }
#define OPS_R16_RM16       { OP_REG,   OP_MEM,  OP_NONE, OP_NONE }
#define OPS_REG_IMM8       { OP_REG,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_REG_IMM16      { OP_REG,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_REG_IMM32      { OP_REG,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_RM8_IMM8       { OP_MEM,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_RM16_IMM16     { OP_MEM,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_RM32_IMM32     { OP_MEM,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_RM16_SEG       { OP_MEM,   OP_SEG,  OP_NONE, OP_NONE }
#define OPS_RM32_SEG       { OP_MEM,   OP_SEG,  OP_NONE, OP_NONE }
#define OPS_SEG_RM16       { OP_SEG,   OP_MEM,  OP_NONE, OP_NONE }
#define OPS_SEG_RM32       { OP_SEG,   OP_MEM,  OP_NONE, OP_NONE }
#define OPS_REG_MOFFS8     { OP_REG,   OP_MOFFS,OP_NONE, OP_NONE }
#define OPS_REG_MOFFS16    { OP_REG,   OP_MOFFS,OP_NONE, OP_NONE }
#define OPS_REG_MOFFS32    { OP_REG,   OP_MOFFS,OP_NONE, OP_NONE }
#define OPS_MOFFS8_REG     { OP_MOFFS, OP_REG,  OP_NONE, OP_NONE }
#define OPS_MOFFS16_REG    { OP_MOFFS, OP_REG,  OP_NONE, OP_NONE }
#define OPS_MOFFS32_REG    { OP_MOFFS, OP_REG,  OP_NONE, OP_NONE }
#define OPS_RM8            { OP_MEM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_IMM8           { OP_IMM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_IMM16          { OP_IMM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_IMM32          { OP_IMM,   OP_NONE, OP_NONE, OP_NONE }
#define OPS_RM32_R32       { OP_MEM,   OP_REG,  OP_NONE, OP_NONE }
#define OPS_R32_RM32       { OP_REG,   OP_MEM,  OP_NONE, OP_NONE }
#define OPS_RM32_IMM8      { OP_MEM,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_R32_IMM32      { OP_REG,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_R8_IMM8        { OP_REG,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_R16_IMM16      { OP_REG,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_RM16_IMM8      { OP_MEM,   OP_IMM,  OP_NONE, OP_NONE }
#define OPS_RM_REG_IMM     { OP_MEM,   OP_REG,  OP_IMM,  OP_NONE }
#define OPS_REG_RM_IMM     { OP_REG,   OP_MEM,  OP_IMM,  OP_NONE }
#define OPS_IMM_IMM        { OP_IMM,   OP_IMM,  OP_NONE, OP_NONE }


/* ════════════════════════════════════════════════════════════════════════════
 *  MNEMONIC TABLE STRUCTURE
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    U32                  mnemonic;
    const PU8            name;
    ASM_PREFIX           prefix;
    ASM_OPCODE_PREFIX    opcode_prefix;
    U8                   opcode[2];
    ASM_ENCODING_TYPE    encoding;
    ASM_OPERAND_TYPE     operand[4];
    ASM_OPERAND_COUNT    operand_count;
    ASM_OPERAND_SIZE     size;
    ASM_REGS             reg_fixed;
    ASM_MODRM_EXTENSION  modrm_ext;
    BOOL                 has_modrm;
    ASM_PROCESSOR        processor;
    ASM_DOC_STATUS       status;
    ASM_CPU_MODE         mode;
    ASM_MEMORY_ACCESS    memory;
    ASM_RELTYPE          rel_type;
    ASM_LOCK_FPU_TYPE    lock_type;
    ASM_OPCODE_EXTENSION opcode_ext;
    ASM_FLAGS            tested_flags;
    ASM_FLAGS            modified_flags;
    ASM_FLAGS            defined_flags;
    ASM_FLAGS            undefined_flags;
    ASM_FLAGS            flags_value;
    PU8                  description;
} ASM_MNEMONIC_TABLE;


/* ════════════════════════════════════════════════════════════════════════════
 *  AST OPERAND
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    ASM_REGS  base_reg;
    ASM_REGS  index_reg;
    S32       scale;
    S32       displacement;
    ASM_REGS  segment;
    PU8       symbol_name;
} ASM_NODE_MEM;

typedef struct {
    ASM_OPERAND_TYPE  type;
    ASM_OPERAND_SIZE  size;
    union {
        ASM_REGS      reg;
        U32           immediate;
        ASM_NODE_MEM *mem_ref;
    };
} ASM_OPERAND;

#define MAX_OPERANDS 4


/* ════════════════════════════════════════════════════════════════════════════
 *  AST NODE
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct ASM_NODE {
    ASM_NODE_TYPE type;
    U32           line;
    U32           col;
    union {
        struct {
            const ASM_MNEMONIC_TABLE *table_entry;
            ASM_OPERAND operands[MAX_OPERANDS];
            U32         operand_count;
            BOOL        shift_by_cl;
        } instr;
        struct { PU8 name; } label;
        struct { PASM_VAR var; } data;
        struct { ASM_DIRECTIVE section; } dir;
        struct { U32 value; } raw;
        struct { U32 origin; } org;
        struct {
            PU8          count_expr;
            ASM_VAR_TYPE fill_type;
            PU8          fill_raw;
        } times;
    };
} ASM_NODE, *PASM_NODE;

#define MAX_NODES 65536

typedef struct {
    U32       len;
    PASM_NODE nodes[MAX_NODES];
} ASM_AST_ARRAY;


/* ════════════════════════════════════════════════════════════════════════════
 *  TOKEN CURSOR
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    ASM_TOK_ARRAY *arr;
    U32            pos;
} TOK_CURSOR;

PASM_TOK TOK_PEEK(TOK_CURSOR *cur);
PASM_TOK TOK_ADVANCE(TOK_CURSOR *cur);
BOOL     TOK_AT_END(TOK_CURSOR *cur);
BOOL     TOK_MATCH(TOK_CURSOR *cur, ASM_TOKEN_TYPE type);
BOOL     TOK_EXPECT(TOK_CURSOR *cur, ASM_TOKEN_TYPE type, PU8 ctx);
VOID     TOK_SKIP_EOL(TOK_CURSOR *cur);


/* ════════════════════════════════════════════════════════════════════════════
 *  PUBLIC API
 * ════════════════════════════════════════════════════════════════════════════ */
ASTRAC_RESULT  START_ASSEMBLING();

PASM_INFO      PREPROCESS_ASM();
PASM_INFO      PREPROCESS_C();
VOID           DESTROY_ASM_INFO(PASM_INFO info);

ASM_TOK_ARRAY *LEX(PASM_INFO info);
VOID           DESTROY_TOK_ARR(ASM_TOK_ARRAY *toks);

ASM_AST_ARRAY *ASM_BUILD_AST(ASM_TOK_ARRAY *toks);
VOID           DESTROY_AST_ARR(ASM_AST_ARRAY *ast);

BOOLEAN        VERIFY_AST(ASM_AST_ARRAY *ast, PASM_INFO info);
BOOLEAN        OPTIMIZE(ASM_AST_ARRAY *ast);
BOOLEAN        GEN_BINARY(ASM_AST_ARRAY *ast, PASM_INFO info);


#endif /* ASSEMBLER_H */
