/*
 * OBJDUMP.c — ACFH binary inspection tool.
 *
 * Dumps AC_FILE_HEADER structure, section table, and binary tables
 * (relocations, etc.) from ACFH-formatted binaries.
 */
#include "AstraC.h"
#include "AC_FH.h"

ASTRAC_RESULT START_OBJDUMP(VOID) {
    ASTRAC_ARGS *cfg = GET_ARGS();
    if (!cfg || !cfg->input_file) {
        AC_PRINTF_ERR("[OBJDUMP] Error: no input file specified.\n");
        return ASTRAC_ERR_ARGS;
    }

    FILE *f = AC_FOPEN(cfg->input_file, MODE_FR);
    if (!f) {
        AC_PRINTF_ERR("[OBJDUMP] Error: failed to open file '%s'\n", cfg->input_file);
        return ASTRAC_ERR_INTERNAL;
    }

    AC_FSEEK(f, 0, SEEK_END);
    U32 fsize = AC_FSIZE(f);
    AC_FSEEK(f, 0, SEEK_SET);

    AC_PRINTF("\n=== AstraC Object Dump ===\n");
    AC_PRINTF("File: %s\n", cfg->input_file);
    AC_PRINTF("File size: %u bytes (0x%X)\n\n", fsize, fsize);

    if (fsize < sizeof(AC_FILE_HEADER)) {
        AC_PRINTF_ERR("[OBJDUMP] Error: File is smaller than ACFH header (%u < %u bytes).\n",
                      fsize, (U32)sizeof(AC_FILE_HEADER));
        AC_FCLOSE(f);
        return ASTRAC_ERR_VERIFY;
    }

    AC_FILE_HEADER hdr;
    AC_MEMZERO(&hdr, sizeof(AC_FILE_HEADER));
    if (AC_FREAD(f, (U8*)&hdr, sizeof(AC_FILE_HEADER)) != sizeof(AC_FILE_HEADER)) {
        AC_PRINTF_ERR("[OBJDUMP] Error: failed to read ACFH header.\n");
        AC_FCLOSE(f);
        return ASTRAC_ERR_INTERNAL;
    }

    /* Check magic */
    if (AC_MEMCMP(hdr.magic, AC_FILE_MAGIC, AC_FILE_MAGIC_LEN) != 0) {
        AC_PRINTF_ERR("[OBJDUMP] Error: Invalid magic '%.4s' (expected '%s'). File is not a valid ACFH binary.\n",
                      hdr.magic, AC_FILE_MAGIC);
        AC_FCLOSE(f);
        return ASTRAC_ERR_VERIFY;
    }

    U16 ver_major = (U16)(hdr.version >> 16);
    U16 ver_minor = (U16)(hdr.version & 0xFFFF);

    U8 flags_buf[128] = {0};
    if (hdr.flags & AC_FLAG_EXECUTABLE) AC_STRCAT(flags_buf, "EXEC ");
    if (hdr.flags & AC_FLAG_DYNAMIC)    AC_STRCAT(flags_buf, "DYNAMIC ");
    if (hdr.flags & AC_FLAG_HAS_RELOCS) AC_STRCAT(flags_buf, "RELOCS ");
    if (flags_buf[0] == '\0')           AC_STRCPY(flags_buf, "NONE");

    AC_PRINTF("-- ACFH Header ---------------------------------------------\n");
    AC_PRINTF("Magic:              %.4s (Valid)\n", hdr.magic);
    AC_PRINTF("Version:            %u.%u (raw: 0x%08X)\n", ver_major, ver_minor, hdr.version);
    AC_PRINTF("Flags:              0x%08X (%s)\n", hdr.flags, flags_buf);
    if (hdr.entry_point_offset == OFFSET_NON_EXISTENT) {
        AC_PRINTF("Entry point offset: None (0xFFFFFFFF)\n");
    } else {
        AC_PRINTF("Entry point offset: 0x%08X\n", hdr.entry_point_offset);
    }
    AC_PRINTF("\n");

    AC_PRINTF("-- Sections ------------------------------------------------\n");
    AC_PRINTF("  Idx  Section   File Offset   Size (bytes)   File Range\n");
    AC_PRINTF("  [0]  .code     0x%08X    %10u     0x%08X - 0x%08X\n",
              hdr.code_offset, hdr.code_size,
              hdr.code_offset, (hdr.code_size ? hdr.code_offset + hdr.code_size - 1 : hdr.code_offset));
    AC_PRINTF("  [1]  .data     0x%08X    %10u     0x%08X - 0x%08X\n",
              hdr.data_offset, hdr.data_size,
              hdr.data_offset, (hdr.data_size ? hdr.data_offset + hdr.data_size - 1 : hdr.data_offset));
    AC_PRINTF("  [2]  .rodata   0x%08X    %10u     0x%08X - 0x%08X\n",
              hdr.rodata_offset, hdr.rodata_size,
              hdr.rodata_offset, (hdr.rodata_size ? hdr.rodata_offset + hdr.rodata_size - 1 : hdr.rodata_offset));
    AC_PRINTF("  [3]  .bss      0x%08X    %10u     (uninitialized)\n",
              hdr.bss_offset, hdr.bss_size);
    AC_PRINTF("\n");

    AC_PRINTF("-- Tables --------------------------------------------------\n");
    if (hdr.reloc_offset != OFFSET_NON_EXISTENT && hdr.reloc_size > 0) {
        U32 reloc_count = hdr.reloc_size / (U32)sizeof(U32);
        AC_PRINTF("Relocation Table: offset 0x%08X, size %u bytes (%u entries)\n",
                  hdr.reloc_offset, hdr.reloc_size, reloc_count);

        if (hdr.reloc_offset + hdr.reloc_size > fsize) {
            AC_PRINTF_WARN("[OBJDUMP] Warning: Relocation table extends beyond file size!\n");
        } else {
            PU32 relocs = (PU32)AC_MAlloc(hdr.reloc_size);
            if (relocs) {
                AC_FSEEK(f, (long)hdr.reloc_offset, SEEK_SET);
                U32 read_bytes = AC_FREAD(f, (U8*)relocs, hdr.reloc_size);
                U32 actual_count = read_bytes / (U32)sizeof(U32);

                AC_PRINTF("  Idx     Instr Offset   Section Relative\n");
                for (U32 i = 0; i < actual_count; i++) {
                    AC_PRINTF("  [%04u]  0x%08X     .code+0x%X\n", i, relocs[i], relocs[i]);
                }
                AC_MFree(relocs);
            }
        }
    } else {
        AC_PRINTF("Relocation Table: None\n");
    }
    AC_PRINTF("\n");

    AC_FCLOSE(f);
    return ASTRAC_OK;
}
