// Dumps asm_mnemonics[] to JSON for use in the web-based assembler reference.
/*
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

Everything is dumped except for the following fields
*/
#include "../ASSEMBLER/ASSEMBLER.h"
#include "../ASSEMBLER/MNEMONICS.h"


#define ENUM_CASE(x) case x: return #x;

static const char *PREFIX_STR(ASM_PREFIX v)
{
    switch (v) {
        ENUM_CASE(PF_NONE)
        ENUM_CASE(PF_66)
        ENUM_CASE(PF_F2)
        ENUM_CASE(PF_F3)
    }
    return "UNKNOWN";
}

static const char *OPCODE_PREFIX_STR(ASM_OPCODE_PREFIX v)
{
    switch (v) {
        ENUM_CASE(PFX_NONE)
        ENUM_CASE(PFX_0F)
    }
    return "UNKNOWN";
}

static const char *ENCODING_STR(ASM_ENCODING_TYPE v)
{
    switch (v) {
        ENUM_CASE(ENC_DIRECT)
        ENUM_CASE(ENC_REG_OPCODE)
        ENUM_CASE(ENC_MODRM)
        ENUM_CASE(ENC_IMM)
    }
    return "UNKNOWN";
}

static const char *OPERAND_STR(ASM_OPERAND_TYPE v)
{
    switch (v) {
        ENUM_CASE(OP_NONE)
        ENUM_CASE(OP_REG)
        ENUM_CASE(OP_MEM)
        ENUM_CASE(OP_IMM)
        ENUM_CASE(OP_SEG)
        ENUM_CASE(OP_PTR)
        ENUM_CASE(OP_FAR)
        ENUM_CASE(OP_MOFFS)
    }
    return "UNKNOWN";
}

static const char *OPERAND_COUNT_STR(ASM_OPERAND_COUNT v)
{
    switch (v) {
        ENUM_CASE(OPN_NONE)
        ENUM_CASE(OPN_ONE)
        ENUM_CASE(OPN_TWO)
        ENUM_CASE(OPN_THREE)
        ENUM_CASE(OPN_FOUR)
    }
    return "UNKNOWN";
}

static const char *SIZE_STR(ASM_OPERAND_SIZE v)
{
    switch (v) {
        ENUM_CASE(SZ_NONE)
        ENUM_CASE(SZ_8BIT)
        ENUM_CASE(SZ_16BIT)
        ENUM_CASE(SZ_32BIT)
    }
    return "UNKNOWN";
}

static const char *REG_STR(ASM_REGS r)
{
    static const char *names[] = {
        "REG_EAX","REG_EBX","REG_ECX","REG_EDX",
        "REG_ESI","REG_EDI","REG_EBP","REG_ESP",

        "REG_AX","REG_BX","REG_CX","REG_DX",
        "REG_SI","REG_DI","REG_BP","REG_SP",

        "REG_AL","REG_AH","REG_BL","REG_BH",
        "REG_CL","REG_CH","REG_DL","REG_DH",

        "REG_IP","REG_EIP",

        "REG_CS","REG_DS","REG_ES","REG_FS","REG_GS","REG_SS",

        "REG_CR0","REG_CR2","REG_CR3","REG_CR4",

        "REG_DR0","REG_DR1","REG_DR2","REG_DR3","REG_DR6","REG_DR7",

        "REG_ST0","REG_ST1","REG_ST2","REG_ST3",
        "REG_ST4","REG_ST5","REG_ST6","REG_ST7",

        "REG_MM0","REG_MM1","REG_MM2","REG_MM3",
        "REG_MM4","REG_MM5","REG_MM6","REG_MM7",

        "REG_XMM0","REG_XMM1","REG_XMM2","REG_XMM3",
        "REG_XMM4","REG_XMM5","REG_XMM6","REG_XMM7"
    };

    if (r == REG_NONE)
        return "REG_NONE";

    if ((unsigned)r < sizeof(names)/sizeof(names[0]))
        return names[r];

    return "UNKNOWN";
}

static const char *MODRM_STR(ASM_MODRM_EXTENSION v)
{
    switch (v) {
        ENUM_CASE(MODRM_NONE)
        ENUM_CASE(MODRM_0)
        ENUM_CASE(MODRM_1)
        ENUM_CASE(MODRM_2)
        ENUM_CASE(MODRM_3)
        ENUM_CASE(MODRM_4)
        ENUM_CASE(MODRM_5)
        ENUM_CASE(MODRM_6)
        ENUM_CASE(MODRM_7)
    }
    return "UNKNOWN";
}

static const char *PROCESSOR_STR(ASM_PROCESSOR v)
{
    switch (v) {
        ENUM_CASE(PROC_ANY)
        ENUM_CASE(PROC_8086)
        ENUM_CASE(PROC_80186)
        ENUM_CASE(PROC_80286)
        ENUM_CASE(PROC_80386)
        ENUM_CASE(PROC_PENTIUM)
    }
    return "UNKNOWN";
}

static const char *STATUS_STR(ASM_DOC_STATUS v)
{
    switch (v) {
        ENUM_CASE(ST_DOCUMENTED)
        ENUM_CASE(ST_MARGINALLY)
        ENUM_CASE(ST_UNDOCUMENTED)
    }
    return "UNKNOWN";
}

static const char *MODE_STR(ASM_CPU_MODE v)
{
    switch (v) {
        ENUM_CASE(MODE_REAL)
        ENUM_CASE(MODE_PROTECTED)
        ENUM_CASE(MODE_V86)
        ENUM_CASE(MODE_ANY)
    }
    return "UNKNOWN";
}

static const char *MEMORY_STR(ASM_MEMORY_ACCESS v)
{
    switch (v) {
        ENUM_CASE(MEM_NONE)
        ENUM_CASE(MEM_READ)
        ENUM_CASE(MEM_WRITE)
        ENUM_CASE(MEM_RW)
    }
    return "UNKNOWN";
}

static const char *RELTYPE_STR(ASM_RELTYPE v)
{
    switch (v) {
        ENUM_CASE(RL_NONE)
        ENUM_CASE(RL_REL8)
        ENUM_CASE(RL_REL16)
        ENUM_CASE(RL_REL32)
    }
    return "UNKNOWN";
}

static const char *LOCK_STR(ASM_LOCK_FPU_TYPE v)
{
    switch (v) {
        ENUM_CASE(X_NONE)
        ENUM_CASE(X_LOCK)
        ENUM_CASE(X_FPU_PUSH)
        ENUM_CASE(X_FPU_POP)
    }
    return "UNKNOWN";
}

static const char *OPCODE_EXT_STR(ASM_OPCODE_EXTENSION v)
{
    switch (v) {
        ENUM_CASE(EX_NONE)
        ENUM_CASE(EX_IMPLICIT)
        ENUM_CASE(EX_PREFIX_0F)
    }
    return "UNKNOWN";
}

static void dump_flags(FILE *f, ASM_FLAGS flags)
{
    int first = 1;

    fprintf(f, "[");

#define FLAG(x) \
    if (flags & x) { \
        if (!first) fprintf(f, ", "); \
        fprintf(f, "\"%s\"", #x); \
        first = 0; \
    }

    FLAG(FLG_O)
    FLAG(FLG_S)
    FLAG(FLG_Z)
    FLAG(FLG_A)
    FLAG(FLG_P)
    FLAG(FLG_C)

#undef FLAG

    fprintf(f, "]");
}

#undef ENUM_CASE
static void opcode_to_string(const ASM_MNEMONIC_TABLE *m,
                             char *out,
                             size_t size)
{
    char bytes[16] = "";

    if (m->opcode[0])
        snprintf(bytes + strlen(bytes),
                 sizeof(bytes) - strlen(bytes),
                 "%02X",
                 m->opcode[0]);

    if (m->opcode[1])
        snprintf(bytes + strlen(bytes),
                 sizeof(bytes) - strlen(bytes),
                 " %02X",
                 m->opcode[1]);

    if (m->has_modrm &&
        m->modrm_ext != MODRM_NONE)
    {
        snprintf(out,
                 size,
                 "%s /%d",
                 bytes,
                 (int)m->modrm_ext);
    }
    else
    {
        snprintf(out,
                 size,
                 "%s",
                 bytes);
    }
}

int main(int argc, char *argv[]) {
    int len = MNEM_COUNT;
    char *filename = "mnemonics.json";
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("[MNEMS_TO_JSON] Failed to open '%s' for writing.\n", filename);
        return 1;
    }

    fprintf(f, "[\n");
    for (int i = 0; i < len; i++) {
        const ASM_MNEMONIC_TABLE *m = &asm_mnemonics[i];

        char opcode[32];

        opcode_to_string(m, opcode, sizeof(opcode));
        fprintf(f, "  {\n");
        fprintf(f, "    \"mnemonic\": %u,\n", m->mnemonic);
        fprintf(f, "    \"name\": \"%s\",\n", m->name);
        fprintf(f,
            "    \"opcode\": [\"%s\"],\n",
            opcode);
        fprintf(f, "    \"prefix\": \"%s\",\n", PREFIX_STR(m->prefix));
        fprintf(f, "    \"opcode_prefix\": \"%s\",\n", OPCODE_PREFIX_STR(m->opcode_prefix));

        fprintf(f, "    \"encoding\": \"%s\",\n", ENCODING_STR(m->encoding));

        fprintf(f,
            "    \"operand\": [\"%s\", \"%s\", \"%s\", \"%s\"],\n",
            OPERAND_STR(m->operand[0]),
            OPERAND_STR(m->operand[1]),
            OPERAND_STR(m->operand[2]),
            OPERAND_STR(m->operand[3]));

        fprintf(f, "    \"operand_count\": \"%s\",\n", OPERAND_COUNT_STR(m->operand_count));
        fprintf(f, "    \"size\": \"%s\",\n", SIZE_STR(m->size));
        fprintf(f, "    \"reg_fixed\": \"%s\",\n", REG_STR(m->reg_fixed));
        fprintf(f, "    \"modrm_ext\": \"%s\",\n", MODRM_STR(m->modrm_ext));

        fprintf(f, "    \"processor\": \"%s\",\n", PROCESSOR_STR(m->processor));
        fprintf(f, "    \"status\": \"%s\",\n", STATUS_STR(m->status));
        fprintf(f, "    \"mode\": \"%s\",\n", MODE_STR(m->mode));
        fprintf(f, "    \"memory\": \"%s\",\n", MEMORY_STR(m->memory));
        fprintf(f, "    \"rel_type\": \"%s\",\n", RELTYPE_STR(m->rel_type));
        fprintf(f, "    \"lock_type\": \"%s\",\n", LOCK_STR(m->lock_type));
        fprintf(f, "    \"opcode_ext\": \"%s\",\n", OPCODE_EXT_STR(m->opcode_ext));

        fprintf(f, "    \"tested_flags\": ");
        dump_flags(f, m->tested_flags);
        fprintf(f, ",\n");

        fprintf(f, "    \"modified_flags\": ");
        dump_flags(f, m->modified_flags);
        fprintf(f, ",\n");

        fprintf(f, "    \"defined_flags\": ");
        dump_flags(f, m->defined_flags);
        fprintf(f, ",\n");

        fprintf(f, "    \"undefined_flags\": ");
        dump_flags(f, m->undefined_flags);
        fprintf(f, ",\n");

        fprintf(f, "    \"flags_value\": ");
        dump_flags(f, m->flags_value);
        fprintf(f, ",\n");

        fprintf(f,
            "    \"description\": \"%s\"\n",
            m->description ? m->description : "");
        if (i < len - 1) {
            fprintf(f, "  },\n");
        } else {
            fprintf(f, "  }\n");
        }
    }
    fprintf(f, "]\n");
    fclose(f);
    return 0;
}