/*
 * ASSEMBLER/MNEMONICS.h — Mnemonic enum + table for the ASTRAC assembler.
 *
 * Requires ASSEMBLER.h to be included first (for all ASM_* types).
 * The instruction list is in ../ASSEMBLER/MNEMONIC_LIST.h (x-macro table).
 */
#ifndef MNEMONICS_H
#define MNEMONICS_H

/* ── PASS 1 : enum values ────────────────────────────────────────────────── */
#define MNEM_NOOPS(id,name,op1,desc)                              id,
#define MNEM_REG_ENC(id,name,op1,ops,sz,desc)                     id,
#define MNEM_IMM(id,name,op1,ops,cnt,sz,desc)                     id,
#define MNEM_RM(id,name,op1,ops,cnt,sz,ext,desc)                  id,
#define MNEM_RM_NF(id,name,op1,ops,cnt,sz,ext,desc)               id,
#define MNEM_RM_P(id,name,pfx,op1,ops,cnt,sz,ext,desc)            id,
#define MNEM_RM_NF_P(id,name,pfx,op1,ops,cnt,sz,ext,desc)         id,
#define MNEM_REL(id,name,op1,rel_type,desc)                       id,
#define MNEM_0F(id,name,op2,ops,cnt,sz,ext,desc)                  id,
#define MNEM_0F_REL(id,name,op2,rel_type,desc)                    id,
#define MNEMONIC(id,name,prefix,opcode,enc,ops,cnt,sz,   \
                 reg_fixed,ext,has_modrm,proc,status,    \
                 mode,mem,rel,lock,opcode_ext,            \
                 tst_f,mod_f,def_f,undef_f,flg_val,desc) id,

typedef enum _ASM_MNEMONIC {
    MNEM_NONE = 0,
    #include "../ASSEMBLER/MNEMONIC_LIST.h"
    MNEM_COUNT,
} ASM_MNEMONIC;

#undef MNEM_NOOPS
#undef MNEM_REG_ENC
#undef MNEM_IMM
#undef MNEM_RM
#undef MNEM_RM_NF
#undef MNEM_RM_P
#undef MNEM_RM_NF_P
#undef MNEM_REL
#undef MNEM_0F
#undef MNEM_0F_REL
#undef MNEMONIC

#define OPCODE(...) { __VA_ARGS__ }

/* ── PASS 2 : table struct literals ─────────────────────────────────────── */

#define MNEM_NOOPS(id,name,op1,desc)                                          \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_DIRECT, OPS_NONE,   \
      OPN_NONE, SZ_NONE, REG_NONE, MODRM_NONE, FALSE,                        \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, RL_NONE,                   \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

#define MNEM_REG_ENC(id,name,op1,ops,sz,desc)                                 \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_REG_OPCODE, ops,    \
      OPN_ONE, sz, REG_NONE, MODRM_NONE, FALSE,                              \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, RL_NONE,                   \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

#define MNEM_IMM(id,name,op1,ops,cnt,sz,desc)                                 \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_IMM, ops,           \
      cnt, sz, REG_NONE, MODRM_NONE, FALSE,                                  \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, RL_NONE,                   \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

#define MNEM_RM(id,name,op1,ops,cnt,sz,ext,desc)                              \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_MODRM, ops,         \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                     \
      X_NONE, EX_NONE, FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE, desc },

#define MNEM_RM_NF(id,name,op1,ops,cnt,sz,ext,desc)                          \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_MODRM, ops,         \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                     \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

#define MNEM_RM_P(id,name,pfx,op1,ops,cnt,sz,ext,desc)                       \
    { id, name, pfx, PFX_NONE, OPCODE(op1,0x00), ENC_MODRM, ops,             \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                     \
      X_NONE, EX_NONE, FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE, desc },

#define MNEM_RM_NF_P(id,name,pfx,op1,ops,cnt,sz,ext,desc)                    \
    { id, name, pfx, PFX_NONE, OPCODE(op1,0x00), ENC_MODRM, ops,             \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                     \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

#define MNEM_REL(id,name,op1,rel_type,desc)                                   \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_IMM, OPS_REL8,      \
      OPN_ONE, SZ_NONE, REG_NONE, MODRM_NONE, FALSE,                         \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, rel_type,                  \
      X_NONE, EX_NONE, FLG_ALL, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

#define MNEM_0F(id,name,op2,ops,cnt,sz,ext,desc)                              \
    { id, name, PF_NONE, PFX_0F, OPCODE(0x0F,op2), ENC_MODRM, ops,           \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_80386, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                   \
      X_NONE, EX_PREFIX_0F, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

#define MNEM_0F_REL(id,name,op2,rel_type,desc)                                \
    { id, name, PF_NONE, PFX_0F, OPCODE(0x0F,op2), ENC_IMM, OPS_REL32,       \
      OPN_ONE, SZ_NONE, REG_NONE, MODRM_NONE, FALSE,                         \
      PROC_80386, ST_DOCUMENTED, MODE_ANY, MEM_NONE, rel_type,                \
      X_NONE, EX_PREFIX_0F, FLG_ALL, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

#define MNEMONIC(id,name,prefix,opcode,enc,ops,cnt,sz,                        \
                 reg_fixed,ext,has_modrm,proc,status,                         \
                 mode,mem,rel,lock,opcode_ext,                                 \
                 tst_f,mod_f,def_f,undef_f,flg_val,desc)                      \
    { id, name, prefix, PFX_NONE, opcode, enc, ops,                           \
      cnt, sz, reg_fixed, ext, has_modrm,                                     \
      proc, status, mode, mem, rel, lock, opcode_ext,                         \
      tst_f, mod_f, def_f, undef_f, flg_val, desc },

static const ASM_MNEMONIC_TABLE asm_mnemonics[] = {
    { MNEM_NONE, "none", PF_NONE, PFX_NONE, OPCODE(0x00,0x00), ENC_DIRECT, OPS_NONE,
      OPN_NONE, SZ_NONE, REG_NONE, MODRM_NONE, FALSE,
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, RL_NONE,
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, "" },
    #include "../ASSEMBLER/MNEMONIC_LIST.h"
};

#undef MNEM_NOOPS
#undef MNEM_REG_ENC
#undef MNEM_IMM
#undef MNEM_RM
#undef MNEM_RM_NF
#undef MNEM_RM_P
#undef MNEM_RM_NF_P
#undef MNEM_REL
#undef MNEM_0F
#undef MNEM_0F_REL
#undef MNEMONIC
#undef OPCODE

#endif /* MNEMONICS_H */
