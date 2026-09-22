/**
 * AstraC.h — Master header for the AstraC hosted port.
 *
 * Pulls in the STDLIB shell layer and defines all pipeline types.
 * Every subsystem includes this (directly or via its own header).
 * Replace STDLIB/*.c on your own OS to use your kernel's facilities.
 */
#ifndef ASTRAC_MAIN_H
#define ASTRAC_MAIN_H

/* ── STDLIB shell ──────────────────────────────────────────────────────── */
#include "STDLIB/TYPEDEF.h"
#include "STDLIB/MEM.h"
#include "STDLIB/IO.h"
#include "STDLIB/STRING.h"
#include "STDLIB/DEBUG.h"
#include "STDLIB/FS_DISK.h"
#include "STDLIB/BINARY.h"

/* ── VERSION ──────────────────────────────────────────────────────────── */
#include "VERSION/VERSION.h"
#define TRADEMARK   "AstraC Compiler, Assembler and Disassembler"

/* ── BUILD MODES ──────────────────────────────────────────────────────── */
typedef enum {
    BUILD_TYPE_NONE            = 0x0000,
    BUILD_TYPE_COMPILE         = 0x0001,   /* AC source  -> AS output          */
    BUILD_TYPE_ASSEMBLE        = 0x0002,   /* AS input  -> binary output       */
    BUILD_TYPE_BUILD           = BUILD_TYPE_COMPILE | BUILD_TYPE_ASSEMBLE, /* full pipeline: AC->AS->BIN */
    BUILD_TYPE_DISASSEMBLE     = 0x0004,   /* binary     -> readable AS        */
    BUILD_TYPE_PREPROCESS_ONLY = 0x0008,   /* preprocess only                   */
    BUILD_TYPE_MNEMONIC_INFO     = 0x0010,   /* show information about a mnemonic */
    BUILD_TYPE_SHOWLINE          = 0x0020,   /* show source lines around a given line */
} BUILD_TYPE;

/* ── RETURN / ERROR CODES ─────────────────────────────────────────────── */
typedef enum _ASTRAC_RESULT {
    ASTRAC_OK               = 0,
    ASTRAC_ERR_ARGS         = 1,
    ASTRAC_ERR_PREPROCESS   = 2,
    ASTRAC_ERR_LEX          = 3,
    ASTRAC_ERR_AST          = 4,
    ASTRAC_ERR_VERIFY       = 5,
    ASTRAC_ERR_OPTIMIZE     = 6,
    ASTRAC_ERR_CODEGEN      = 7,
    ASTRAC_ERR_DISASSEMBLE  = 8,
    ASTRAC_ERR_COMPILE      = 9,
    ASTRAC_ERR_INTERNAL     = 0xFF,
} ASTRAC_RESULT;

/* ── ARCHITECTURE ─────────────────────────────────────────────────────── */
typedef enum {
    ARCH_NONE = 0,
    ARCH_I386,
    ARCH_I286,
} ARCH;

/* ── OUTPUT TYPES ─────────────────────────────────────────────────────── */
typedef enum {
    OUTPUT_NONE = 0,
    OUTPUT_EXE,
    OUTPUT_LIB,
} OUTPUT_TYPE;

/* ── LIMITS ───────────────────────────────────────────────────────────── */
#define MAX_MACROS          1024
#define MAX_INCLUDES        255
#define MAX_INPUT_FILES     255
#define MAX_MACRO_VALUE     255
#define MAX_MACRO_PARAMS    16
#define MAX_MACRO_PARAM_LEN 64
#define BUF_SZ              4096
#define MAX_FILES           MAX_INPUT_FILES
#define PUSH_MAX            32
#define POP_MAX             PUSH_MAX

/* ── PREPROCESSOR MODES ───────────────────────────────────────────────── */
#define ASM_PREPROCESSOR    1
#define C_PREPROCESSOR      2

/* ── MACRO STORAGE ────────────────────────────────────────────────────── */
typedef struct {
    U8 name[MAX_MACRO_VALUE];
    U8 value[MAX_MACRO_VALUE];
    BOOL is_function;
    U32 num_params;
    U8 params[MAX_MACRO_PARAMS][MAX_MACRO_PARAM_LEN];
} MACRO, *PMACRO;

typedef struct {
    PMACRO macros[MAX_MACROS];
    U32 len;
} MACRO_ARR;

BOOL   DEFINE_MACRO(PU8 name, PU8 value, MACRO_ARR *arr);
PMACRO GET_MACRO(PU8 name, MACRO_ARR *arr);
VOID   UNDEFINE_MACRO(PU8 name, MACRO_ARR *arr);
VOID   FREE_MACROS(MACRO_ARR *arr);

/* ── LINE READING ─────────────────────────────────────────────────────── */
BOOL READ_LOGICAL_LINE(FILE *file, U8 *out, U32 out_size);
BOOL IS_EMPTY(PU8 line);

/* ── WARNINGS ─────────────────────────────────────────────────────────── */
/* Returns TRUE when a diagnostic of severity `warning_level` should be
 * emitted (SHARED/WARNINGS.c). */
BOOLEAN WARNING(U8 warning_level);

/* ── ARGUMENT STRUCTURE ───────────────────────────────────────────────── */
typedef struct _ASTRAC_ARGS {
    MACRO_ARR macros;
    U8 stepoff_level;
    BOOL verbose;

    PU8 input_file;
    PU8 outfile;

    BUILD_TYPE build_type;


    ARCH arch;
    OUTPUT_TYPE output_type;
    U32 dsm_bits;
    U32 org;
    PU8 entry_point;
    U32 warning_level;
    BOOL warnings_as_errors;
    BOOL debug;

    /* showline arguments */
    BOOL showline_is_ac;     /* TRUE = show 00.AC, FALSE = show 00.AS */
    U32  showline_ctx;       /* plus/minus context window size */
    U32  showline_start;     /* start line number */
    U32  showline_end;       /* optional end line number (0 = none) */
    BOOL showline_has_end;   /* TRUE if end line was provided */



    // switches defined by preprocessor
    U32 PARSER_TOPLEVEL_LOG_PUSH[PUSH_MAX];
    U32 PARSER_TOPLEVEL_LOG_PUSH_TAIL;
    U32 PARSER_TOPLEVEL_LOG_POP[POP_MAX];
    U32 PARSER_TOPLEVEL_LOG_POP_TAIL;
} ASTRAC_ARGS;


ASTRAC_ARGS *GET_ARGS();
VOID         FREE_ARGS();
ASTRAC_RESULT START_WORKLOAD();
ASTRAC_RESULT START_MNEMONIC_INFO(PU8 mnemonic);

#endif /* ASTRAC_MAIN_H */