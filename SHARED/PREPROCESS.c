/*
 * SHARED/PREPROCESS.c — Preprocessor pipeline stage.
 *
 * Shared between the assembler (ASM_PREPROCESSOR) and compiler (C_PREPROCESSOR).
 * For each input file the preprocessor:
 *   1. Reads lines (coalescing backslash continuations)
 *   2. Strips comments  (; for ASM, // and /* for C)
 *   3. Handles directives: #include, #define, #undef, #ifdef/#ifndef/#elif/#else/#endif, #error, #warning
 *   4. Performs macro substitution on surviving lines
 *   5. Writes the result to a temp file in /TMP/
 *
 * The temp file path is collected in ASM_INFO for the lexer.
 */
#include "../ASSEMBLER/ASSEMBLER.h"


/*
 * ── PREPROCESSOR DIRECTIVE IDENTIFIERS ──────────────────────────────────────
 */
#define MAX_PREPROCESSING_UNITS MAX_FILES

typedef enum {
    IDENT_INCLUDE,
    IDENT_IFDEF,
    IDENT_IFNDEF,
    IDENT_ELIF,
    IDENT_ELSE,
    IDENT_ENDIF,
    IDENT_ERROR,
    IDENT_WARNING,
    IDENT_DEFINE,
    IDENT_UNDEF,
    IDENT_MAX,
} MACRO_IDENT;

typedef struct {
    PU8         str;
    MACRO_IDENT ident;
} MACRO_KW;

static const MACRO_KW macros_kw[] ATTRIB_RODATA = {
    { "#include",  IDENT_INCLUDE  },
    { "#ifdef",    IDENT_IFDEF    },
    { "#ifndef",   IDENT_IFNDEF   },
    { "#elif",     IDENT_ELIF     },
    { "#else",     IDENT_ELSE     },
    { "#endif",    IDENT_ENDIF    },
    { "#error",    IDENT_ERROR    },
    { "#warning",  IDENT_WARNING  },
    { "#define",   IDENT_DEFINE   },
    { "#undef",    IDENT_UNDEF    },
};


/*
 * ── MODULE-LEVEL STATE ──────────────────────────────────────────────────────
 */
static U8        tmp_str[BUF_SZ]  ATTRIB_DATA = { 0 };
static MACRO_ARR glb_macros       ATTRIB_DATA = { 0 };

typedef struct {
    FILE     *file;
    MACRO_ARR macros;
    U32       type;     /* ASM_PREPROCESSOR or C_PREPROCESSOR */
} PREPROCESSING_UNIT;

static PREPROCESSING_UNIT units[MAX_PREPROCESSING_UNITS] ATTRIB_DATA = { 0 };
static U32 unit_cnt ATTRIB_DATA = 0;


/*
 * ── #ifdef / #ifndef / #elif / #else / #endif STACK ─────────────────────────
 */
typedef struct {
    BOOL active;    /* Is the current branch being emitted?          */
    BOOL was_true;  /* Has ANY branch in this #if group been true?   */
} IF_STATE;

#define IF_STACK_MAX 32
static IF_STATE if_stack[IF_STACK_MAX] ATTRIB_DATA = { 0 };
static U32      if_stack_top           ATTRIB_DATA = 0;

STATIC VOID IF_STACK_PUSH(PU8 name, MACRO_ARR *arr, BOOL invert) {
    if (if_stack_top >= IF_STACK_MAX) return;
    IF_STATE s;
    s.active   = invert ? (GET_MACRO(name, arr) == NULL)
                        : (GET_MACRO(name, arr) != NULL);
    s.was_true = s.active;
    if_stack[if_stack_top++] = s;
}

STATIC VOID IF_STACK_ELIF(PU8 name, MACRO_ARR *arr) {
    if (if_stack_top == 0) return;
    IF_STATE *s = &if_stack[if_stack_top - 1];
    if (s->was_true) {
        s->active = FALSE;
    } else {
        s->active   = (GET_MACRO(name, arr) != NULL);
        s->was_true = s->active;
    }
}

STATIC VOID IF_STACK_ELSE() {
    if (if_stack_top == 0) return;
    IF_STATE *s = &if_stack[if_stack_top - 1];
    s->active   = !s->was_true;
    s->was_true = TRUE;
}

STATIC VOID IF_STACK_POP() {
    if (if_stack_top > 0) if_stack_top--;
}

STATIC BOOL IF_STACK_IS_ACTIVE() {
    for (U32 i = 0; i < if_stack_top; i++) {
        if (!if_stack[i].active) return FALSE;
    }
    return TRUE;
}


/*
 * ── Forward declarations ────────────────────────────────────────────────────
 */
VOID FREE_PREPROCESSING_UNITS();


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  PREPROCESSOR EXPRESSION EVALUATOR
 * ════════════════════════════════════════════════════════════════════════════
 *  Simple recursive-descent evaluator for constant integer expressions.
 *  Supports: decimal, 0xHEX, 0bBINARY literals, +, -, *, /, %, ~,
 *  <<, >>, |, &, ^, unary -, unary ~, and parentheses.
 *  Returns TRUE if the string was a valid expression (result in *out).
 */

/* Forward declarations for recursive descent */
STATIC BOOL PP_EVAL_EXPR(PU8 *pp, S32 *out);

STATIC VOID PP_SKIP_SPACES(PU8 *pp) {
    while (**pp == ' ' || **pp == '\t') (*pp)++;
}

STATIC BOOL PP_IS_DIGIT(U8 c) { return c >= '0' && c <= '9'; }
STATIC BOOL PP_IS_HEX(U8 c) {
    return PP_IS_DIGIT(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

STATIC BOOL PP_EVAL_ATOM(PU8 *pp, S32 *out) {
    PP_SKIP_SPACES(pp);

    /* Parenthesized sub-expression */
    if (**pp == '(') {
        (*pp)++;
        if (!PP_EVAL_EXPR(pp, out)) return FALSE;
        PP_SKIP_SPACES(pp);
        if (**pp != ')') return FALSE;
        (*pp)++;
        return TRUE;
    }

    /* Unary minus */
    if (**pp == '-') {
        (*pp)++;
        if (!PP_EVAL_ATOM(pp, out)) return FALSE;
        *out = -(*out);
        return TRUE;
    }

    /* Unary plus */
    if (**pp == '+') {
        (*pp)++;
        return PP_EVAL_ATOM(pp, out);
    }

    /* Bitwise NOT */
    if (**pp == '~') {
        (*pp)++;
        if (!PP_EVAL_ATOM(pp, out)) return FALSE;
        *out = ~(*out);
        return TRUE;
    }

    /* Hex literal: 0x... */
    if (**pp == '0' && ((*pp)[1] == 'x' || (*pp)[1] == 'X')) {
        *pp += 2;
        if (!PP_IS_HEX(**pp)) return FALSE;
        U32 v = 0;
        while (PP_IS_HEX(**pp)) {
            U8 c = **pp;
            U32 d = PP_IS_DIGIT(c) ? c - '0'
                  : (c >= 'a') ? c - 'a' + 10 : c - 'A' + 10;
            v = v * 16 + d;
            (*pp)++;
        }
        *out = (S32)v;
        return TRUE;
    }

    /* Binary literal: 0b... */
    if (**pp == '0' && ((*pp)[1] == 'b' || (*pp)[1] == 'B')) {
        *pp += 2;
        if (**pp != '0' && **pp != '1') return FALSE;
        U32 v = 0;
        while (**pp == '0' || **pp == '1') {
            v = v * 2 + (**pp - '0');
            (*pp)++;
        }
        *out = (S32)v;
        return TRUE;
    }

    /* Decimal literal */
    if (PP_IS_DIGIT(**pp)) {
        U32 v = 0;
        while (PP_IS_DIGIT(**pp)) {
            v = v * 10 + (**pp - '0');
            (*pp)++;
        }
        *out = (S32)v;
        return TRUE;
    }

    return FALSE;   /* Not a numeric expression */
}

/* Multiplicative: *, /, % */
STATIC BOOL PP_EVAL_MUL(PU8 *pp, S32 *out) {
    if (!PP_EVAL_ATOM(pp, out)) return FALSE;
    for (;;) {
        PP_SKIP_SPACES(pp);
        U8 op = **pp;
        if (op != '*' && op != '/' && op != '%') break;
        (*pp)++;
        S32 rhs;
        if (!PP_EVAL_ATOM(pp, &rhs)) return FALSE;
        if (op == '*') *out *= rhs;
        else if (rhs == 0) return FALSE;   /* div by zero */
        else if (op == '/') *out /= rhs;
        else                *out %= rhs;
    }
    return TRUE;
}

/* Additive: +, - */
STATIC BOOL PP_EVAL_ADD(PU8 *pp, S32 *out) {
    if (!PP_EVAL_MUL(pp, out)) return FALSE;
    for (;;) {
        PP_SKIP_SPACES(pp);
        U8 op = **pp;
        if (op != '+' && op != '-') break;
        (*pp)++;
        S32 rhs;
        if (!PP_EVAL_MUL(pp, &rhs)) return FALSE;
        if (op == '+') *out += rhs;
        else           *out -= rhs;
    }
    return TRUE;
}

/* Shift: <<, >> */
STATIC BOOL PP_EVAL_SHIFT(PU8 *pp, S32 *out) {
    if (!PP_EVAL_ADD(pp, out)) return FALSE;
    for (;;) {
        PP_SKIP_SPACES(pp);
        if (**pp == '<' && (*pp)[1] == '<') {
            *pp += 2;
            S32 rhs; if (!PP_EVAL_ADD(pp, &rhs)) return FALSE;
            *out = (S32)((U32)*out << rhs);
        } else if (**pp == '>' && (*pp)[1] == '>') {
            *pp += 2;
            S32 rhs; if (!PP_EVAL_ADD(pp, &rhs)) return FALSE;
            *out = (S32)((U32)*out >> rhs);
        } else break;
    }
    return TRUE;
}

/* Bitwise AND */
STATIC BOOL PP_EVAL_AND(PU8 *pp, S32 *out) {
    if (!PP_EVAL_SHIFT(pp, out)) return FALSE;
    for (;;) {
        PP_SKIP_SPACES(pp);
        if (**pp != '&') break;
        (*pp)++;
        S32 rhs; if (!PP_EVAL_SHIFT(pp, &rhs)) return FALSE;
        *out &= rhs;
    }
    return TRUE;
}

/* Bitwise XOR */
STATIC BOOL PP_EVAL_XOR(PU8 *pp, S32 *out) {
    if (!PP_EVAL_AND(pp, out)) return FALSE;
    for (;;) {
        PP_SKIP_SPACES(pp);
        if (**pp != '^') break;
        (*pp)++;
        S32 rhs; if (!PP_EVAL_AND(pp, &rhs)) return FALSE;
        *out ^= rhs;
    }
    return TRUE;
}

/* Bitwise OR — top-level expression */
STATIC BOOL PP_EVAL_EXPR(PU8 *pp, S32 *out) {
    if (!PP_EVAL_XOR(pp, out)) return FALSE;
    for (;;) {
        PP_SKIP_SPACES(pp);
        if (**pp != '|') break;
        (*pp)++;
        S32 rhs; if (!PP_EVAL_XOR(pp, &rhs)) return FALSE;
        *out |= rhs;
    }
    return TRUE;
}

/*
 * Try to evaluate `value` as a constant integer expression.
 * If successful, overwrite `value` with the decimal result string and return TRUE.
 * If the string is not a pure numeric expression, leave it untouched and return FALSE.
 */
STATIC BOOL PP_TRY_EVAL(PU8 value) {
    if (!value || !*value) return FALSE;

    PU8 p = value;
    S32 result;
    if (!PP_EVAL_EXPR(&p, &result)) return FALSE;

    /* Ensure ALL input was consumed (otherwise it's not a pure expression) */
    PP_SKIP_SPACES(&p);
    if (*p != '\0') return FALSE;

    AC_SPRINTF(value, "%d", result);
    return TRUE;
}


/* ════════════════════════════════════════════════════════════════════════════
 *  MACRO TABLE
 * ════════════════════════════════════════════════════════════════════════════ */

BOOL DEFINE_MACRO(PU8 name, PU8 value, MACRO_ARR *arr) {
    if (!name || arr->len >= MAX_MACROS) return FALSE;
    /* Update existing entry */
    for (U32 i = 0; i < arr->len; i++) {
        if (arr->macros[i] && AC_STRCMP(arr->macros[i]->name, name) == 0) {
            AC_STRNCPY(arr->macros[i]->value, value ? value : "", MAX_MACRO_VALUE - 1);
            arr->macros[i]->value[MAX_MACRO_VALUE - 1] = '\0';
            return TRUE;
        }
    }
    PMACRO m = (PMACRO)AC_MAlloc(sizeof(MACRO));
    if (!m) return FALSE;
    AC_MEMZERO(m, sizeof(MACRO));
    AC_STRNCPY(m->name, name, MAX_MACRO_VALUE - 1);
    m->name[MAX_MACRO_VALUE - 1] = '\0';
    if (value) {
        AC_STRNCPY(m->value, value, MAX_MACRO_VALUE - 1);
        m->value[MAX_MACRO_VALUE - 1] = '\0';
    }
    arr->macros[arr->len++] = m;
    return TRUE;
}

PMACRO GET_MACRO(PU8 name, MACRO_ARR *arr) {
    if (!name || !arr) return NULLPTR;
    for (U32 i = 0; i < arr->len; i++) {
        if (arr->macros[i] && AC_STRCMP(arr->macros[i]->name, name) == 0)
            return arr->macros[i];
    }
    return NULLPTR;
}

VOID UNDEFINE_MACRO(PU8 name, MACRO_ARR *arr) {
    if (!name || !arr) return;
    for (U32 i = 0; i < arr->len; i++) {
        if (arr->macros[i] && AC_STRCMP(arr->macros[i]->name, name) == 0) {
            AC_MFree(arr->macros[i]);
            /* Shift remaining entries down */
            for (U32 j = i; j < arr->len - 1; j++)
                arr->macros[j] = arr->macros[j + 1];
            arr->macros[--arr->len] = NULLPTR;
            return;
        }
    }
}

VOID FREE_MACROS(MACRO_ARR *arr) {
    if (!arr) return;
    for (U32 i = 0; i < arr->len; i++) {
        if (arr->macros[i]) AC_MFree(arr->macros[i]);
        arr->macros[i] = NULLPTR;
    }
    arr->len = 0;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  COMMENT REMOVAL
 * ════════════════════════════════════════════════════════════════════════════
 */

VOID REMOVE_COMMENTS(PU8 line, U32 mode) {
    static BOOL in_block_comment = FALSE;

    U32 len       = (U32)AC_STRLEN(line);
    U32 write_pos = 0;
    BOOL in_string = FALSE;
    BOOL in_char   = FALSE;

    for (U32 i = 0; i < len; i++) {
        U8 c    = line[i];
        U8 next = (i + 1 < len) ? line[i + 1] : '\0';

        /* ── ASM mode: ';' starts a line comment ── */
        if (mode == ASM_PREPROCESSOR) {
            if (c == ';') break;
            line[write_pos++] = c;
            continue;
        }

        /* ── C mode ── */
        if (in_block_comment) {
            if (c == '*' && next == '/') { in_block_comment = FALSE; i++; }
            continue;
        }

        if (!in_char && c == '"' && (i == 0 || line[i - 1] != '\\'))
            in_string = !in_string;
        else if (!in_string && c == '\'' && (i == 0 || line[i - 1] != '\\'))
            in_char = !in_char;

        if (!in_string && !in_char) {
            if (c == '/' && next == '*') { in_block_comment = TRUE; i++; continue; }
            if (c == '/' && next == '/') break;
        }

        line[write_pos++] = c;
    }

    line[write_pos] = '\0';
}


/* ════════════════════════════════════════════════════════════════════════════
 *  LINE UTILITIES
 * ════════════════════════════════════════════════════════════════════════════ */

BOOL IS_EMPTY(PU8 line) {
    AC_STR_TRIM((char *)line);
    return (AC_STRLEN(line) == 0);
}

/*
 * Extract the first token after a directive keyword.
 * "#define FOO bar"  + IDENT_DEFINE  -> "FOO"
 */
STATIC PU8 EXTRACT_NAME(PU8 line, MACRO_IDENT type) {
    PU8 p = line + AC_STRLEN(macros_kw[type].str);
    while (*p == ' ' || *p == '\t') p++;

    PU8 start = p;
    while (*p && *p != ' ' && *p != '\t') p++;

    U32 len = (U32)(p - start);
    if (len == 0) return NULLPTR;

    PU8 name = (PU8)AC_MAlloc(len + 1);
    if (!name) return NULLPTR;
    AC_STRNCPY(name, start, len);
    name[len] = '\0';
    AC_STR_TRIM((char *)name);
    return name;
}

/*
 * Extract the value portion after the name.
 * "#define FOO bar baz"  -> "bar baz"
 */
STATIC PU8 EXTRACT_VALUE(PU8 line, MACRO_IDENT type) {
    PU8 p = line + AC_STRLEN(macros_kw[type].str);
    while (*p == ' ' || *p == '\t') p++;
    while (*p && *p != ' ' && *p != '\t') p++;   /* skip name  */
    while (*p == ' ' || *p == '\t') p++;          /* skip space */

    U32 len = (U32)AC_STRLEN(p);
    if (len == 0) return NULLPTR;

    PU8 value = (PU8)AC_MAlloc(len + 1);
    if (!value) return NULLPTR;
    AC_STRNCPY(value, p, len);
    value[len] = '\0';
    AC_STR_TRIM((char *)value);
    return value;
}

/*
 * Extract everything after the keyword as a single value.
 * "#include <FOO.INC>"  -> "<FOO.INC>"
 */
STATIC PU8 EXTRACT_SINGLE_VALUE(PU8 line, MACRO_IDENT type) {
    PU8 p = line + AC_STRLEN(macros_kw[type].str);
    while (*p == ' ' || *p == '\t') p++;

    U32 len = (U32)AC_STRLEN(p);
    if (len == 0) return NULLPTR;

    PU8 value = (PU8)AC_MAlloc(len + 1);
    if (!value) return NULLPTR;
    AC_STRNCPY(value, p, len);
    value[len] = '\0';
    AC_STR_TRIM((char *)value);
    return value;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  MACRO SUBSTITUTION
 * ════════════════════════════════════════════════════════════════════════════
 *  Replace all known macro names in `line` with their values.
 *  Word-boundary-aware: only replaces when the match is NOT embedded inside
 *  a larger identifier (e.g. macro "ONE" must not match inside "LOOPNE").
 *  Global macros (from -D flags) are checked first, then per-unit macros.
 */

/* Helper: TRUE if c is alphanumeric or underscore (identifier char). */
STATIC BOOL IS_IDENT_CHAR(U8 c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9') || c == '_';
}

/* Word-boundary-aware replacement of all occurrences of `name` with `value`
 * in `line`. Modifies `line` in-place (up to BUF_SZ). */
STATIC VOID REPLACE_WORD(PU8 line, PU8 name, PU8 value) {
    U32 name_len  = (U32)AC_STRLEN(name);
    U32 value_len = (U32)AC_STRLEN(value);
    if (!name_len) return;

    U8  out[BUF_SZ];
    U32 out_len = 0;
    PU8 p = line;

    while (*p) {
        PU8 found = AC_STRSTR(p, name);
        if (!found) {
            /* Copy remainder */
            U32 rest = (U32)AC_STRLEN(p);
            if (out_len + rest < BUF_SZ) {
                AC_STRNCPY(out + out_len, p, BUF_SZ - out_len - 1);
                out_len += rest;
            }
            break;
        }

        /* Check word boundaries around the match */
        BOOL at_word_boundary = TRUE;
        if (found > line && IS_IDENT_CHAR(*(found - 1)))
            at_word_boundary = FALSE;
        if (at_word_boundary && IS_IDENT_CHAR(*(found + name_len)))
            at_word_boundary = FALSE;

        if (at_word_boundary) {
            /* Copy text before match */
            U32 before = (U32)(found - p);
            if (out_len + before + value_len < BUF_SZ) {
                AC_STRNCPY(out + out_len, p, before);
                out_len += before;
                AC_STRNCPY(out + out_len, value, BUF_SZ - out_len - 1);
                out_len += value_len;
            }
            p = found + name_len;
        } else {
            /* Not a word boundary — copy the match text literally and move past */
            U32 skip = (U32)(found - p) + name_len;
            if (out_len + skip < BUF_SZ) {
                AC_STRNCPY(out + out_len, p, skip);
                out_len += skip;
            }
            p = found + name_len;
        }
    }
    out[out_len] = '\0';
    AC_STRNCPY(line, out, BUF_SZ - 1);
    line[BUF_SZ - 1] = '\0';
}

STATIC VOID REPLACE_MACROS_IN_LINE(PU8 line, MACRO_ARR *local) {
    /* Globals first */
    for (U32 i = 0; i < glb_macros.len; i++) {
        PMACRO m = glb_macros.macros[i];
        if (!m || !AC_STRSTR(line, m->name)) continue;
        REPLACE_WORD(line, m->name, m->value);
    }
    /* Then local/per-unit */
    for (U32 i = 0; i < local->len; i++) {
        PMACRO m = local->macros[i];
        if (!m || !AC_STRSTR(line, m->name)) continue;
        REPLACE_WORD(line, m->name, m->value);
    }
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  LOGICAL LINE READER
 * ════════════════════════════════════════════════════════════════════════════
 *  Joins backslash-continued physical lines into one logical line.
 *  Returns TRUE when a line was read, FALSE at EOF.
 */
BOOL READ_LOGICAL_LINE(FILE *file, U8 *out, U32 out_size) {
    if (!file || !out || out_size == 0) return FALSE;

    U8  phys[BUF_SZ];
    U32 out_len = 0;
    out[0] = '\0';

    while (TRUE) {
        if (!AC_FILE_GET_LINE(file, phys, sizeof(phys)))
            return (out_len > 0);

        /* trim trailing CR/LF and whitespace */
        U32 p = (U32)AC_STRLEN(phys);
        while (p > 0 && (phys[p-1] == '\n' || phys[p-1] == '\r')) p--;
        phys[p] = '\0';
        while (p > 0 && (phys[p-1] == ' '  || phys[p-1] == '\t')) { p--; phys[p] = '\0'; }

        BOOL continuation = (p > 0 && phys[p-1] == '\\');
        if (continuation) { phys[--p] = '\0'; }

        /* append to output buffer */
        if (out_len + p + 2 >= out_size) return FALSE;   /* overflow */

        if (out_len > 0) { out[out_len++] = ' '; out[out_len] = '\0'; }
        AC_STRNCAT(out, phys, out_size - out_len - 1);
        out_len = (U32)AC_STRLEN(out);

        if (!continuation) return TRUE;
    }
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  INCLUDE PATH RESOLUTION
 * ════════════════════════════════════════════════════════════════════════════
 *  Relative #include paths are resolved against the directory of the file that
 *  contains the #include directive (NOT the process working directory).
 */

/* Extract the directory portion of `path` (everything up to the last
 * '/' or '\\').  A path with no separator yields an empty directory. */
STATIC VOID PP_GET_DIR(PU8 path, U8 *out, U32 out_sz) {
    if (!path || !out || out_sz == 0) return;
    U32 len = (U32)AC_STRLEN(path);
    U32 i   = len;
    while (i > 0 && path[i-1] != '/' && path[i-1] != '\\') i--;
    if (i >= out_sz) i = out_sz - 1;
    AC_STRNCPY(out, path, i);
    out[i] = '\0';
}

/* TRUE if `p` is an absolute path (drive-letter or rooted). */
STATIC BOOL PP_IS_ABS(PU8 p) {
    if (!p || !*p) return FALSE;
    if (p[0] == '/' || p[0] == '\\') return TRUE;
    return (p[0] && p[1] == ':' && (p[2] == '/' || p[2] == '\\'));
}

/* Join `dir` and `rel` into `out`, inserting a separator if needed. */
STATIC VOID PP_JOIN_PATH(PU8 dir, PU8 rel, U8 *out, U32 out_sz) {
    if (!rel || !out || out_sz == 0) return;
    if (PP_IS_ABS(rel)) {
        AC_STRNCPY(out, rel, out_sz - 1);
        out[out_sz - 1] = '\0';
        return;
    }
    U32 dl = (dir && *dir) ? (U32)AC_STRLEN(dir) : 0;
    U32 i  = 0;
    if (dl > 0) {
        if (dl >= out_sz) dl = out_sz - 1;
        AC_STRNCPY(out, dir, dl);
        i = dl;
        if (i > 0 && out[i-1] != '/' && out[i-1] != '\\' && i < out_sz - 1)
            out[i++] = '/';
    }
    if (i < out_sz)
        AC_STRNCPY(out + i, rel, out_sz - i - 1);
    out[out_sz - 1] = '\0';
}

/* Normalize `path` in place: resolve `.` and `..` components, unify
 * separators to '/', and strip redundant separators.  Handles a leading
 * '/', '\\', or drive-letter prefix. */
STATIC VOID PP_NORMALIZE(PU8 path) {
    if (!path || !*path) return;
    U8  out[BUF_SZ];
    U32 n   = 0;
    U32 i   = 0;
    U32 len = (U32)AC_STRLEN(path);
    BOOL abs = FALSE;

    /* Preserve root: leading separator or drive letter. */
    if (path[0] == '/' || path[0] == '\\') {
        out[n++] = '/';
        i = 1;
        abs = TRUE;
    } else if (path[0] && path[1] == ':') {
        out[n++] = path[0];
        out[n++] = ':';
        i = 2;
        if (path[i] == '/' || path[i] == '\\') i++;
        abs = TRUE;
    }
    U32 root = abs ? 1 : 0;   /* first index a component may occupy */

    while (i <= len) {
        U32 j = i;
        while (j < len && path[j] != '/' && path[j] != '\\') j++;
        U32 clen = j - i;

        if (clen == 0 || (clen == 1 && path[i] == '.')) {
            /* skip empty or "." component */
        } else if (clen == 2 && path[i] == '.' && path[i+1] == '.') {
            /* Find start of the last component in `out`. */
            U32 last = n;
            while (last > root && out[last-1] != '/') last--;
            U32 last_len = n - last;
            if (last_len == 2 && out[last] == '.' && out[last+1] == '.') {
                /* top is already '..' -> push another '..' */
                if (n > 0 && out[n-1] != '/') out[n++] = '/';
                if (n + 2 < BUF_SZ) { out[n++] = '.'; out[n++] = '.'; }
            } else if (last > root) {
                /* pop the last regular component */
                n = last;
                if (n > root) n--;
            } else {
                /* empty stack -> push '..' */
                if (n > 0 && out[n-1] != '/') out[n++] = '/';
                if (n + 2 < BUF_SZ) { out[n++] = '.'; out[n++] = '.'; }
            }
        } else {
            if (n > 0 && out[n-1] != '/') out[n++] = '/';
            if (clen >= BUF_SZ - n - 1) clen = (BUF_SZ > n + 2) ? BUF_SZ - n - 2 : 0;
            if (clen > 0) { AC_STRNCPY(out + n, path + i, clen); n += clen; }
        }
        i = j + 1;
    }

    if (n == 0) { out[n++] = '.'; }
    out[n] = '\0';
    AC_STRNCPY(path, out, BUF_SZ - 1);
    path[BUF_SZ - 1] = '\0';
}

/* ── #include recursion guard ──────────────────────────────────────────────── */
/* A depth limit converts runaway (unguarded) circular includes into a clean
 * error instead of a stack overflow.  Guarded headers break cycles via
 * #ifndef/#define, which the normal conditional machinery already handles. */
#define PP_INCLUDE_DEPTH_MAX 64
static U32  pp_open_depth ATTRIB_DATA = 0;

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  PREPROCESS A SINGLE FILE  (recursive for #include)
 * ════════════════════════════════════════════════════════════════════════════
 */
STATIC BOOL PREPROCESS_FILE(FILE *file, FILE *tmp_file, MACRO_ARR *mcr,
                            PREPROCESSING_UNIT *unit, PU8 cur_dir) {
    U8 buf[BUF_SZ] = { 0 };
    while (READ_LOGICAL_LINE(file, buf, sizeof(buf))) {
        REMOVE_COMMENTS(buf, unit->type);
        if (IS_EMPTY(buf)) continue;

        BOOL active = IF_STACK_IS_ACTIVE();

        /* ── Preprocessor directive? ───────────────────────────── */
        if (buf[0] == '#') {
            U32 matched = U32_MAX;
            for (U32 i = 0; i < IDENT_MAX; i++) {
                if (AC_STRISTR((const char *)buf, (const char *)macros_kw[i].str)) { matched = i; break; }
            }

            if (matched == U32_MAX) {
                AC_PRINTF("[PP] Unknown preprocessor directive: %s\n", buf);
                return FALSE;
            }

            PU8 name  = EXTRACT_NAME(buf, matched);
            PU8 value = EXTRACT_VALUE(buf, matched);

            switch (matched) {
                case IDENT_IFDEF:  IF_STACK_PUSH(name, mcr, FALSE); break;
                case IDENT_IFNDEF: IF_STACK_PUSH(name, mcr, TRUE);  break;
                case IDENT_ELIF:   IF_STACK_ELIF(name, mcr);        break;
                case IDENT_ELSE:   IF_STACK_ELSE();                  break;
                case IDENT_ENDIF:  IF_STACK_POP();                   break;

                default:
                    if (!active) break;    /* skip remaining directives in dead branch */

                    switch (matched) {
                        /* ── #define ── */
                        case IDENT_DEFINE:
                            /* Expand macros in the value, then try to evaluate
                               it as a constant integer expression. */
                            if (value) {
                                REPLACE_MACROS_IN_LINE(value, mcr);
                                PP_TRY_EVAL(value);
                            }
                            if (!DEFINE_MACRO(name, value, mcr)) {
                                AC_PRINTF("[PP] Failed to define macro '%s'\n", name);
                                AC_MFree(name); AC_MFree(value);
                                return FALSE;
                            }
                            break;

                        /* ── #undef ── */
                        case IDENT_UNDEF:
                            UNDEFINE_MACRO(name, mcr);
                            break;

                        /* ── #include ── */
                        case IDENT_INCLUDE: {
                            AC_MFree(value);
                            value = EXTRACT_SINGLE_VALUE(buf, matched);
                            if (!value || !*value) {
                                AC_PRINTF("[PP] Invalid include path: %s\n", buf);
                                AC_MFree(name);
                                return FALSE;
                            }

                            PU8  path      = value;
                            BOOL is_system = FALSE;
                            if (*path == '<') { is_system = TRUE; path++; PU8 e = AC_STRCHR(path, '>'); if (e) *e = '\0'; }
                            else if (*path == '"') { path++; PU8 e = AC_STRCHR(path, '"'); if (e) *e = '\0'; }

                            if (is_system) {
                                AC_STRCPY(tmp_str, "/ATOS/SYS_SRC");
                                if (path[0] != '/') AC_STRCAT(tmp_str, "/");
                                AC_STRCAT(tmp_str, path);
                            } else {
                                /* Resolve relative includes against the
                                 * including file's directory. */
                                PP_JOIN_PATH(cur_dir, path, tmp_str, sizeof(tmp_str));
                            }
                            PP_NORMALIZE(tmp_str);

                            if (pp_open_depth >= PP_INCLUDE_DEPTH_MAX) {
                                AC_PRINTF("[PP] #include nesting too deep (>= %u): %s\n",
                                          PP_INCLUDE_DEPTH_MAX, tmp_str);
                                AC_MFree(name); AC_MFree(value);
                                return FALSE;
                            }

                            FILE *inc = AC_FOPEN(tmp_str, MODE_R | MODE_FAT32);
                            if (!inc) {
                                AC_PRINTF("[PP] Cannot open include: %s\n", tmp_str);
                                AC_MFree(name); AC_MFree(value);
                                return FALSE;
                            }

                            if (GET_ARGS()->verbose) AC_PRINTF("[PP] Including: %s\n", tmp_str);

                            /* Compute the included file's directory for nested
                             * includes, then recurse. */
                            U8 sub_dir[BUF_SZ] = { 0 };
                            PP_GET_DIR(tmp_str, sub_dir, sizeof(sub_dir));
                            PP_NORMALIZE(sub_dir);
                            pp_open_depth++;

                            BOOL ok = PREPROCESS_FILE(inc, tmp_file, mcr, unit, sub_dir);

                            pp_open_depth--;
                            AC_FCLOSE(inc);
                            if (!ok) { AC_MFree(name); AC_MFree(value); return FALSE; }
                        } break;

                        /* ── #error ── */
                        case IDENT_ERROR: {
                            AC_MFree(value);
                            value = EXTRACT_SINGLE_VALUE(buf, matched);
                            AC_PRINTF("[PP] ERROR: %s\n", value ? value : "(empty)");
                            AC_MFree(name); AC_MFree(value);
                            return FALSE;
                        }

                        /* ── #warning ── */
                        case IDENT_WARNING: {
                            AC_MFree(value);
                            value = EXTRACT_SINGLE_VALUE(buf, matched);
                            AC_PRINTF("[PP] WARNING: %s\n", value ? value : "(empty)");
                        } break;
                    }
                    break;
            }

            if (name)  AC_MFree(name);
            if (value) AC_MFree(value);
            continue;   /* directive lines are never written to output */
        }

        /* ── Skip inactive code ── */
        if (!active) continue;

        /* ── Macro-expand and write to output ── */
        REPLACE_MACROS_IN_LINE(buf, mcr);
        U32 len = (U32)AC_STRLEN(buf);
        if (len > 0 && len + 1 < BUF_SZ) {
            buf[len] = '\n';
            buf[len + 1] = '\0';
            if (!AC_FWRITE(tmp_file, buf, len + 1))
                return FALSE;
        }
    }

    return TRUE;
}


/*
 * ── Cleanup ─────────────────────────────────────────────────────────────────
 */
VOID FREE_PREPROCESSING_UNITS() {
    for (U32 i = 0; i < unit_cnt; i++) {
        PREPROCESSING_UNIT *u = &units[i];
        if (u->file) AC_FCLOSE(u->file);
        FREE_MACROS(&u->macros);
        AC_MEMSET(u, 0, sizeof(PREPROCESSING_UNIT));
    }
    unit_cnt = 0;
    FREE_MACROS(&glb_macros);
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  PREPROCESS_ASM — top-level entry point
 * ════════════════════════════════════════════════════════════════════════════
 *  1. Create /TMP/00.AS
 *  2. Open the input file
 *  3. Run PREPROCESS_FILE (recursive for includes)
 *  4. Store the temp path in info->tmp_files[0]
 *  Returns the filled ASM_INFO, or NULL on fatal error.
 */
PASM_INFO PREPROCESS_ASM() {
    unit_cnt = 0;
    if_stack_top = 0;

    ASTRAC_ARGS *args = GET_ARGS();
    if (!args || !args->input_file) {
        AC_PRINTF("[AS PREPROC] Error: no input file.\n");
        return NULLPTR;
    }

    PASM_INFO info = (PASM_INFO)AC_MAlloc(sizeof(ASM_INFO));
    if (!info) return NULLPTR;
    AC_MEMZERO(info, sizeof(ASM_INFO));

    /* Seed global macros from command-line -D flags */
    for (U32 i = 0; i < args->macros.len; i++) {
        PMACRO m = args->macros.macros[i];
        if (m) DEFINE_MACRO(m->name, m->value, &glb_macros);
    }

    /* ── Build temp file path ── */
    U8 tmp_path[32];
    AC_SPRINTF(tmp_path, "/TMP/00.AS");
    if (AC_FILE_EXISTS(tmp_path)) {
        if (!AC_FILE_DELETE(tmp_path)) {
            AC_PRINTF("[PP] Cannot delete existing temp file: %s\n", tmp_path);
            FREE_PREPROCESSING_UNITS();
            return info;
        }
    }
    if (!AC_FILE_CREATE(tmp_path)) {
        AC_PRINTF("[PP] Cannot create temp file: %s\n", tmp_path);
        FREE_PREPROCESSING_UNITS();
        return info;
    }

    FILE *tmp = AC_FOPEN(tmp_path, MODE_RA | MODE_FAT32);
    if (!tmp) {
        AC_PRINTF("[PP] Cannot open temp file: %s\n", tmp_path);
        FREE_PREPROCESSING_UNITS();
        return info;
    }

    /* ── Open source file ── */
    PREPROCESSING_UNIT *unit = &units[unit_cnt];
    unit->type = ASM_PREPROCESSOR;
    unit->file = AC_FOPEN(args->input_file, MODE_R | MODE_FAT32);
    if (!unit->file) {
        AC_PRINTF("[PP] Cannot open input: %s\n", args->input_file);
        AC_FCLOSE(tmp);
        FREE_PREPROCESSING_UNITS();
        return NULLPTR;
    }

    if (args->verbose) AC_PRINTF("[PP] Processing: %s -> %s\n", args->input_file, tmp_path);

    /* ── Run preprocessor ── */
    {
        U8 top_dir[BUF_SZ] = { 0 };
        PP_GET_DIR(args->input_file, top_dir, sizeof(top_dir));
        PP_NORMALIZE(top_dir);
        pp_open_depth = 0;
        BOOL ok = PREPROCESS_FILE(unit->file, tmp, &unit->macros, unit, top_dir);
        pp_open_depth--;
        AC_FCLOSE(tmp);

        if (!ok) {
            AC_PRINTF("[PP] Failed on file: %s\n", args->input_file);
            FREE_PREPROCESSING_UNITS();
            return info;
        }
    }

    info->tmp_files[info->tmp_file_count++] = AC_STRDUP(tmp_path);
    unit_cnt++;

    FREE_PREPROCESSING_UNITS();
    return info;
}

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  PREPROCESS_C — top-level entry point for AC source files
 *  Identical to PREPROCESS_ASM but uses C_PREPROCESSOR mode so the
 *  comment-stripping pass handles // and /* instead of ;.
 * ════════════════════════════════════════════════════════════════════════════
 */
PASM_INFO PREPROCESS_C() {
    unit_cnt = 0;
    if_stack_top = 0;

    ASTRAC_ARGS *args = GET_ARGS();
    if (!args || !args->input_file) {
        AC_PRINTF("[C PREPROC] Error: no input file.\n");
        return NULLPTR;
    }

    PASM_INFO info = (PASM_INFO)AC_MAlloc(sizeof(ASM_INFO));
    if (!info) return NULLPTR;
    AC_MEMZERO(info, sizeof(ASM_INFO));

    /* Seed global macros from command-line -D flags */
    for (U32 i = 0; i < args->macros.len; i++) {
        PMACRO m = args->macros.macros[i];
        if (m) DEFINE_MACRO(m->name, m->value, &glb_macros);
    }

    /* ── Build temp file path ── */
    U8 tmp_path[32];
    #ifdef _WIN32
    AC_SPRINTF(tmp_path, "C:\\TMP\\00.AC");
    #else
    AC_SPRINTF(tmp_path, "/TMP/00.AC");
    #endif
    if (AC_FILE_EXISTS(tmp_path)) {
        if (!AC_FILE_DELETE(tmp_path)) {
            AC_PRINTF("[PP] Cannot delete existing temp file: %s\n", tmp_path);
            FREE_PREPROCESSING_UNITS();
            return info;
        }
    }
    if (!AC_FILE_CREATE(tmp_path)) {
        AC_PRINTF("[PP] Cannot create temp file: %s\n", tmp_path);
        FREE_PREPROCESSING_UNITS();
        return info;
    }

    FILE *tmp = AC_FOPEN(tmp_path, MODE_RA | MODE_FAT32);
    if (!tmp) {
        AC_PRINTF("[PP] Cannot open temp file: %s\n", tmp_path);
        FREE_PREPROCESSING_UNITS();
        return info;
    }

    /* ── Open source file ── */
    PREPROCESSING_UNIT *unit = &units[unit_cnt];
    unit->type = C_PREPROCESSOR;
    unit->file = AC_FOPEN(args->input_file, MODE_R | MODE_FAT32);
    if (!unit->file) {
        AC_PRINTF("[PP] Cannot open input: %s\n", args->input_file);
        AC_FCLOSE(tmp);
        FREE_PREPROCESSING_UNITS();
        return NULLPTR;
    }

    if (args->verbose) AC_PRINTF("[PP] Processing: %s -> %s\n", args->input_file, tmp_path);

    /* ── Run preprocessor ── */
    {
        U8 top_dir[BUF_SZ] = { 0 };
        PP_GET_DIR(args->input_file, top_dir, sizeof(top_dir));
        PP_NORMALIZE(top_dir);
        pp_open_depth = 0;
        BOOL ok = PREPROCESS_FILE(unit->file, tmp, &unit->macros, unit, top_dir);
        pp_open_depth--;
        AC_FCLOSE(tmp);

        if (!ok) {
            AC_PRINTF("[PP] Failed on file: %s\n", args->input_file);
            FREE_PREPROCESSING_UNITS();
            return info;
        }
    }

    info->tmp_files[info->tmp_file_count++] = AC_STRDUP(tmp_path);
    unit_cnt++;

    FREE_PREPROCESSING_UNITS();
    return info;
}
