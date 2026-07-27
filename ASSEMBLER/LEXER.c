/*
 * ASSEMBLER/LEXER.c — Assembler lexer.
 *
 * Reads preprocessed .AS source files and emits a flat ASM_TOK_ARRAY.
 * Recognises: directives, registers, variable types, mnemonics, numbers,
 * string literals, symbols, and generic identifiers.
 */
#include "ASSEMBLER.h"
#include "MNEMONICS.h"

/* ════════════════════════════════════════════════════════════════════════════
 *  KEYWORD TABLES
 * ════════════════════════════════════════════════════════════════════════════ */

STATIC CONST KEYWORD s_registers[] ATTRIB_RODATA = {
    { "eax", REG_EAX }, { "ebx", REG_EBX }, { "ecx", REG_ECX }, { "edx", REG_EDX },
    { "esi", REG_ESI }, { "edi", REG_EDI }, { "ebp", REG_EBP }, { "esp", REG_ESP },
    { "ax",  REG_AX  }, { "bx",  REG_BX  }, { "cx",  REG_CX  }, { "dx",  REG_DX  },
    { "si",  REG_SI  }, { "di",  REG_DI  }, { "bp",  REG_BP  }, { "sp",  REG_SP  },
    { "ah",  REG_AH  }, { "al",  REG_AL  }, { "bh",  REG_BH  }, { "bl",  REG_BL  },
    { "ch",  REG_CH  }, { "cl",  REG_CL  }, { "dh",  REG_DH  }, { "dl",  REG_DL  },
    { "cs",  REG_CS  }, { "ds",  REG_DS  }, { "es",  REG_ES  },
    { "fs",  REG_FS  }, { "gs",  REG_GS  }, { "ss",  REG_SS  },
    { "cr0", REG_CR0 }, { "cr2", REG_CR2 }, { "cr3", REG_CR3 }, { "cr4", REG_CR4 },
    { "dr0", REG_DR0 }, { "dr1", REG_DR1 }, { "dr2", REG_DR2 }, { "dr3", REG_DR3 },
    { "dr6", REG_DR6 }, { "dr7", REG_DR7 },
    { "st0", REG_ST0 }, { "st1", REG_ST1 }, { "st2", REG_ST2 }, { "st3", REG_ST3 },
    { "st4", REG_ST4 }, { "st5", REG_ST5 }, { "st6", REG_ST6 }, { "st7", REG_ST7 },
    { "mm0", REG_MM0 }, { "mm1", REG_MM1 }, { "mm2", REG_MM2 }, { "mm3", REG_MM3 },
    { "mm4", REG_MM4 }, { "mm5", REG_MM5 }, { "mm6", REG_MM6 }, { "mm7", REG_MM7 },
    { "xmm0", REG_XMM0 }, { "xmm1", REG_XMM1 }, { "xmm2", REG_XMM2 }, { "xmm3", REG_XMM3 },
    { "xmm4", REG_XMM4 }, { "xmm5", REG_XMM5 }, { "xmm6", REG_XMM6 }, { "xmm7", REG_XMM7 },
    { NULLPTR, REG_NONE }
};

STATIC CONST KEYWORD s_directives[] ATTRIB_RODATA = {
    { "data",   DIR_DATA          },
    { "rodata", DIR_RODATA        },
    { "code",   DIR_CODE          },
    { "use32",  DIR_CODE_TYPE_32  },
    { "use16",  DIR_CODE_TYPE_16  },
    { "org",    DIR_ORG           },
    { "times",  DIR_TIMES         },
    { NULLPTR, 0 }
};

/* 2-char symbols listed before 1-char so try_symbol_at matches longest first */
STATIC CONST KEYWORD s_symbols[] ATTRIB_RODATA = {
    { "<<", SYM_SHL           },
    { ">>", SYM_SHR           },
    { "$$", SYM_DOLLAR_DOLLAR },
    { ",",  SYM_COMMA         },
    { ":",  SYM_COLON         },
    { ";",  SYM_SEMICOLON     },
    { "[",  SYM_LBRACKET      },
    { "]",  SYM_RBRACKET      },
    { "(",  SYM_LPAREN        },
    { ")",  SYM_RPAREN        },
    { "+",  SYM_PLUS          },
    { "-",  SYM_MINUS         },
    { "*",  SYM_ASTERISK      },
    { "/",  SYM_SLASH         },
    { "%",  SYM_PERCENT       },
    { "&",  SYM_AND           },
    { "|",  SYM_OR            },
    { "^",  SYM_XOR           },
    { "~",  SYM_TILDE         },
    { "=",  SYM_EQUALS        },
    { ".",  SYM_DOT           },
    { "@",  SYM_AT            },
    { "$",  SYM_DOLLAR        },
    { NULLPTR, SYM_NONE       }
};

STATIC CONST KEYWORD s_var_types[] ATTRIB_RODATA = {
    { "DB",    TYPE_BYTE  },
    { "DW",    TYPE_WORD  },
    { "DD",    TYPE_DWORD },
    { "BYTE",  TYPE_BYTE  },
    { "WORD",  TYPE_WORD  },
    { "DWORD", TYPE_DWORD },
    { "REAL4", TYPE_FLOAT },
    { "PTR",   TYPE_PTR   },
    { "NEAR",  TYPE_NEAR  },
    { "FAR",   TYPE_FAR   },
    { NULLPTR, TYPE_NONE  }
};

const KEYWORD *get_registers()      { return s_registers;  }
const KEYWORD *get_directives()     { return s_directives; }
const KEYWORD *get_symbols()        { return s_symbols;    }
const KEYWORD *get_variable_types() { return s_var_types;  }

const char *TOKEN_TYPE_STR(ASM_TOKEN_TYPE t) {
    switch (t) {
        case TOK_NONE:        return "NONE";
        case TOK_EOL:         return "EOL";
        case TOK_EOF:         return "EOF";
        case TOK_LABEL:       return "LABEL";
        case TOK_LOCAL_LABEL: return "LOCAL_LABEL";
        case TOK_MNEMONIC:    return "MNEMONIC";
        case TOK_REGISTER:    return "REGISTER";
        case TOK_NUMBER:      return "NUMBER";
        case TOK_STRING:      return "STRING";
        case TOK_SYMBOL:      return "SYMBOL";
        case TOK_IDENT_VAR:   return "IDENT_VAR";
        case TOK_IDENTIFIER:  return "IDENTIFIER";
        case TOK_DIRECTIVE:   return "DIRECTIVE";
        default:              return "UNKNOWN";
    }
}


/* ════════════════════════════════════════════════════════════════════════════
 *  INTERNAL HELPERS
 * ════════════════════════════════════════════════════════════════════════════ */

STATIC BOOL LEX_ADD_TOKEN(ASM_TOK_ARRAY *arr, ASM_TOKEN_TYPE type,
                          PU8 txt, U32 uval, U32 line, U32 col) {
    if (arr->len >= MAX_TOKENS) return FALSE;
    PASM_TOK tok = (PASM_TOK)AC_MAlloc(sizeof(ASM_TOK));
    if (!tok) return FALSE;
    AC_MEMSET(tok, 0, sizeof(ASM_TOK));
    tok->type    = type;
    tok->txt     = AC_STRDUP(txt && *txt ? txt : "");
    tok->num_val = uval;
    tok->line    = line;
    tok->col     = col;
    arr->toks[arr->len++] = tok;
    return TRUE;
}

STATIC CONST KEYWORD *lookup_kw(CONST KEYWORD *arr, PU8 s) {
    if (!s || !*s) return NULLPTR;
    for (U32 i = 0; arr[i].value; i++)
        if (AC_STRICMP(s, arr[i].value) == 0) return &arr[i];
    return NULLPTR;
}

/* Parse decimal / 0xHEX / 0bBINARY literal. Returns TRUE and stores value. */
STATIC BOOL lex_parse_num(PU8 p, U32 *val_out) {
    BOOL neg = FALSE;
    if (*p == '+') p++;
    else if (*p == '-') { neg = TRUE; p++; }
    if (!*p) return FALSE;

    U32 v = 0;
    if (*p == '0' && (p[1]=='x'||p[1]=='X')) {
        p += 2; if (!*p) return FALSE;
        for (; *p; p++) {
            U8 c = *p; U32 d;
            if (c>='0'&&c<='9') d=c-'0';
            else if (c>='a'&&c<='f') d=c-'a'+10;
            else if (c>='A'&&c<='F') d=c-'A'+10;
            else return FALSE;
            v=(v<<4)|d;
        }
    } else if (*p == '0' && (p[1]=='b'||p[1]=='B')) {
        p += 2; if (!*p) return FALSE;
        for (; *p; p++) {
            if (*p!='0'&&*p!='1') return FALSE;
            v=(v<<1)|(*p-'0');
        }
    } else {
        if (!IS_DIGIT(*p)) return FALSE;
        for (; *p; p++) {
            if (!IS_DIGIT(*p)) return FALSE;
            v = v*10 + (*p-'0');
        }
    }
    if (neg) v = (U32)(-(S32)v);
    *val_out = v;
    return TRUE;
}

/*
 * Match a 2-char then 1-char symbol at buf[pos].  sym_symbols table has
 * 2-char entries first, so we try those before falling back to 1-char.
 */
STATIC BOOL try_sym(CONST U8 *buf, U32 pos, U32 len,
                    U32 *sym_enum, U32 *sym_len) {
    /* 2-char */
    if (pos + 1 < len) {
        U8 two[3] = { buf[pos], buf[pos+1], '\0' };
        for (U32 i = 0; s_symbols[i].value; i++) {
            if (s_symbols[i].value[1] != '\0'
                && AC_STRCMP((PU8)two, s_symbols[i].value) == 0) {
                *sym_enum = s_symbols[i].enum_val;
                *sym_len  = 2;
                return TRUE;
            }
        }
    }
    /* 1-char */
    for (U32 i = 0; s_symbols[i].value; i++) {
        if (s_symbols[i].value[0] == buf[pos] && s_symbols[i].value[1] == '\0') {
            *sym_enum = s_symbols[i].enum_val;
            *sym_len  = 1;
            return TRUE;
        }
    }
    return FALSE;
}

/* Classify and emit the accumulated text token in tokbuf. */
STATIC BOOL emit_tok(ASM_TOK_ARRAY *arr, PU8 tokbuf, U32 line, U32 col) {
    if (!tokbuf || !*tokbuf) return TRUE;

    const KEYWORD *kw;

    kw = lookup_kw(s_registers, tokbuf);
    if (kw) return LEX_ADD_TOKEN(arr, TOK_REGISTER, tokbuf, kw->enum_val, line, col);

    kw = lookup_kw(s_var_types, tokbuf);
    if (kw) return LEX_ADD_TOKEN(arr, TOK_IDENT_VAR, tokbuf, kw->enum_val, line, col);

    /* Mnemonic lookup in x-macro generated table */
    U32 mnem_count = sizeof(asm_mnemonics) / sizeof(asm_mnemonics[0]);
    for (U32 i = 0; i < mnem_count; i++) {
        if (asm_mnemonics[i].name && AC_STRICMP(tokbuf, asm_mnemonics[i].name) == 0)
            return LEX_ADD_TOKEN(arr, TOK_MNEMONIC, tokbuf,
                                 asm_mnemonics[i].mnemonic, line, col);
    }

    U32 num_val = 0;
    if (lex_parse_num(tokbuf, &num_val))
        return LEX_ADD_TOKEN(arr, TOK_NUMBER, tokbuf, num_val, line, col);

    return LEX_ADD_TOKEN(arr, TOK_IDENTIFIER, tokbuf, 0, line, col);
}


/* ════════════════════════════════════════════════════════════════════════════
 *  MAIN LEXER
 * ════════════════════════════════════════════════════════════════════════════ */

ASM_TOK_ARRAY *LEX(PASM_INFO info) {
    if (!info || info->tmp_file_count == 0) return NULLPTR;

    ASM_TOK_ARRAY *res = (ASM_TOK_ARRAY *)AC_MAlloc(sizeof(ASM_TOK_ARRAY));
    if (!res) return NULLPTR;
    AC_MEMZERO(res, sizeof(ASM_TOK_ARRAY));

    U8  linebuf[BUF_SZ];
    U8  tokbuf[BUF_SZ];
    U32 lineno = 0;
    BOOL ok = TRUE;

    for (U32 f = 0; f < info->tmp_file_count && ok; f++) {
        FILE *file = AC_FOPEN(info->tmp_files[f], MODE_R);
        if (!file) {
            AC_PRINTF("[AS LEX] Cannot open '%s'\n", info->tmp_files[f]);
            ok = FALSE;
            break;
        }

        while (ok && AC_FILE_GET_LINE(file, linebuf, sizeof(linebuf))) {
            lineno++;
            U32 len = (U32)AC_STRLEN(linebuf);
            while (len > 0 && (linebuf[len-1]=='\n'||linebuf[len-1]=='\r'))
                linebuf[--len] = '\0';

            U32 i = 0, tok_len = 0;

            while (i <= len && ok) {
                U8 c = linebuf[i];

                /* end of line or ; comment: flush buffer, emit EOL */
                if (c == '\0' || c == ';') {
                    if (tok_len > 0) {
                        tokbuf[tok_len] = '\0';
                        if (!emit_tok(res, tokbuf, lineno, i+1-tok_len)) { ok=FALSE; break; }
                        tok_len = 0;
                    }
                    ok = LEX_ADD_TOKEN(res, TOK_EOL, "", 0, lineno, i+1);
                    break;
                }

                /* whitespace: flush accumulated token */
                if (c == ' ' || c == '\t') {
                    if (tok_len > 0) {
                        tokbuf[tok_len] = '\0';
                        if (!emit_tok(res, tokbuf, lineno, i+1-tok_len)) { ok=FALSE; break; }
                        tok_len = 0;
                    }
                    i++; continue;
                }

                /* string literal */
                if (c == '"') {
                    if (tok_len > 0) {
                        tokbuf[tok_len] = '\0';
                        if (!emit_tok(res, tokbuf, lineno, i+1-tok_len)) { ok=FALSE; break; }
                        tok_len = 0;
                    }
                    U32 str_col = i+1;
                    i++; tok_len = 0;
                    while (i < len && linebuf[i] != '"') {
                        if (tok_len < BUF_SZ-1) tokbuf[tok_len++] = linebuf[i];
                        i++;
                    }
                    if (i < len && linebuf[i] == '"') i++;
                    tokbuf[tok_len] = '\0';
                    ok = LEX_ADD_TOKEN(res, TOK_STRING, tokbuf, 0, lineno, str_col);
                    tok_len = 0;
                    continue;
                }

                /* leading dot: section directive (.code, .data, .use32 …) */
                if (c == '.' && tok_len == 0) {
                    i++; tok_len = 0;
                    while (i < len && (IS_ALPHA(linebuf[i])||IS_DIGIT(linebuf[i]))) {
                        if (tok_len < BUF_SZ-1) tokbuf[tok_len++] = linebuf[i++];
                    }
                    tokbuf[tok_len] = '\0';
                    const KEYWORD *kw = lookup_kw(s_directives, tokbuf);
                    U32 col = i+1-tok_len-1;
                    if (kw)
                        ok = LEX_ADD_TOKEN(res, TOK_DIRECTIVE, tokbuf, kw->enum_val, lineno, col);
                    else
                        ok = LEX_ADD_TOKEN(res, TOK_IDENTIFIER, tokbuf, 0, lineno, col);
                    tok_len = 0;
                    continue;
                }

                /* symbol check (try 2-char before 1-char) */
                U32 sym_enum = 0, sym_len = 0;
                if (try_sym(linebuf, i, len, &sym_enum, &sym_len)) {
                    if (tok_len > 0) {
                        tokbuf[tok_len] = '\0';
                        if (!emit_tok(res, tokbuf, lineno, i+1-tok_len)) { ok=FALSE; break; }
                        tok_len = 0;
                    }
                    U8 sym_txt[3] = { linebuf[i], sym_len>1?linebuf[i+1]:'\0', '\0' };
                    ok = LEX_ADD_TOKEN(res, TOK_SYMBOL, sym_txt, sym_enum, lineno, i+1);
                    i += sym_len;
                    continue;
                }

                /* accumulate into token buffer */
                if (tok_len < BUF_SZ-1) tokbuf[tok_len++] = c;
                i++;
            }
        }
        AC_FCLOSE(file);
    }

    if (!ok) {
        DESTROY_TOK_ARR(res);
        return NULLPTR;
    }

    LEX_ADD_TOKEN(res, TOK_EOF, "", 0, lineno, 1);
    return res;
}
