/*
 * OBJDUMP.c — ACFH binary inspection tool.
 *
 * Dumps AC_FILE_HEADER structure, section table, and binary tables
 * (relocations, function export tables, etc.) from ACFH-formatted binaries.
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

    OBJDUMP_MODE mode = cfg->objdump_mode;
    BOOL show_hdr   = (mode == OBJDUMP_MODE_ALL || mode == OBJDUMP_MODE_HEADER);
    BOOL show_reloc = (mode == OBJDUMP_MODE_ALL || mode == OBJDUMP_MODE_TABLES || mode == OBJDUMP_MODE_RELOCS);
    BOOL show_funcs = (mode == OBJDUMP_MODE_ALL || mode == OBJDUMP_MODE_TABLES || mode == OBJDUMP_MODE_FUNCS);
    BOOL show_imports = (mode == OBJDUMP_MODE_ALL || mode == OBJDUMP_MODE_TABLES || mode == OBJDUMP_MODE_IMPORTS);

    if (show_hdr) {
        U16 ver_major = (U16)(hdr.version >> 16);
        U16 ver_minor = (U16)(hdr.version & 0xFFFF);

        U8 flags_buf[128] = {0};
        if (hdr.flags & AC_FLAG_EXECUTABLE) AC_STRCAT(flags_buf, "EXEC ");
        if (hdr.flags & AC_FLAG_DYNAMIC)    AC_STRCAT(flags_buf, "DYNAMIC ");
        if (hdr.flags & AC_FLAG_HAS_RELOCS) AC_STRCAT(flags_buf, "RELOCS ");
        if (hdr.flags & AC_FLAG_HAS_FUNCS)  AC_STRCAT(flags_buf, "FUNCS ");
        if (hdr.flags & AC_FLAG_HAS_IMPORTS)AC_STRCAT(flags_buf, "IMPORTS ");
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
    }

    if (show_reloc) {
        AC_PRINTF("-- Relocation Table ----------------------------------------\n");
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
    }

    if (show_funcs) {
        AC_PRINTF("-- Function Export Table -----------------------------------\n");
        if (hdr.func_table_offset != OFFSET_NON_EXISTENT && hdr.func_table_size > 0) {
            if (hdr.func_table_offset + hdr.func_table_size > fsize) {
                AC_PRINTF_WARN("[OBJDUMP] Warning: Function export table extends beyond file size!\n");
            } else {
                AC_FSEEK(f, (long)hdr.func_table_offset, SEEK_SET);
                AC_FUNC_TABLE_HDR fhdr;
                if (AC_FREAD(f, (U8*)&fhdr, sizeof(AC_FUNC_TABLE_HDR)) == sizeof(AC_FUNC_TABLE_HDR)) {
                    AC_PRINTF("Function Table: offset 0x%08X, entries %u, string table %u bytes\n",
                              hdr.func_table_offset, fhdr.entry_count, fhdr.string_table_size);

                    U32 entries_size = fhdr.entry_count * sizeof(AC_FUNC_ENTRY);
                    AC_FUNC_ENTRY *entries = (AC_FUNC_ENTRY*)AC_MAlloc(entries_size);
                    PU8 strtab = (PU8)AC_MAlloc(fhdr.string_table_size + 1);

                    if (entries && strtab) {
                        AC_FREAD(f, (U8*)entries, entries_size);
                        AC_FREAD(f, strtab, fhdr.string_table_size);
                        strtab[fhdr.string_table_size] = '\0';

                        AC_PRINTF("  Idx     Address       ParamSize  Flags      Function Name\n");
                        for (U32 i = 0; i < fhdr.entry_count; i++) {
                            PU8 fname = (entries[i].name_offset < fhdr.string_table_size)
                                      ? (strtab + entries[i].name_offset) : (PU8)"<invalid>";
                            U8 flags_str[32] = "CDECL";
                            if (entries[i].flags & AC_FUNC_FLAG_STDCALL) AC_STRCPY(flags_str, "STDCALL");
                            if (entries[i].flags & AC_FUNC_FLAG_VARIADIC) AC_STRCAT(flags_str, "|VAR");

                            AC_PRINTF("  [%04u]  0x%08X     %5u B     %-10s %s\n",
                                      i, entries[i].address, entries[i].param_size, flags_str, fname);
                        }
                    }
                    if (entries) AC_MFree(entries);
                    if (strtab)  AC_MFree(strtab);
                }
            }
        } else {
            AC_PRINTF("Function Table: None\n");
        }
        AC_PRINTF("\n");
    }

    if (show_imports) {
        AC_PRINTF("-- Import Table --------------------------------------------\n");
        if (hdr.import_table_offset != OFFSET_NON_EXISTENT && hdr.import_table_size > 0) {
            if (hdr.import_table_offset + hdr.import_table_size > fsize) {
                AC_PRINTF_WARN("[OBJDUMP] Warning: Import table extends beyond file size!\n");
            } else {
                AC_FSEEK(f, (long)hdr.import_table_offset, SEEK_SET);
                AC_IMPORT_TABLE_HDR ihdr;
                if (AC_FREAD(f, (U8*)&ihdr, sizeof(AC_IMPORT_TABLE_HDR)) == sizeof(AC_IMPORT_TABLE_HDR)) {
                    AC_PRINTF("Import Table: offset 0x%08X, entries %u, string table %u bytes\n",
                              hdr.import_table_offset, ihdr.entry_count, ihdr.string_table_size);

                    U32 entries_size = ihdr.entry_count * sizeof(AC_IMPORT_ENTRY);
                    AC_IMPORT_ENTRY *entries = (AC_IMPORT_ENTRY*)AC_MAlloc(entries_size);
                    PU8 strtab = (PU8)AC_MAlloc(ihdr.string_table_size + 1);

                    if (entries && strtab) {
                        AC_FREAD(f, (U8*)entries, entries_size);
                        AC_FREAD(f, strtab, ihdr.string_table_size);
                        strtab[ihdr.string_table_size] = '\0';

                        AC_PRINTF("  Idx     Library Name        Function Name       Patch Offset\n");
                        for (U32 i = 0; i < ihdr.entry_count; i++) {
                            PU8 lname = (entries[i].lib_name_offset < ihdr.string_table_size)
                                      ? (strtab + entries[i].lib_name_offset) : (PU8)"<invalid>";
                            PU8 fname = (entries[i].func_name_offset < ihdr.string_table_size)
                                      ? (strtab + entries[i].func_name_offset) : (PU8)"<invalid>";

                            AC_PRINTF("  [%04u]  %-18s  %-18s  0x%08X\n",
                                      i, lname, fname, entries[i].patch_offset);
                        }
                    }
                    if (entries) AC_MFree(entries);
                    if (strtab)  AC_MFree(strtab);
                }
            }
        } else {
            AC_PRINTF("Import Table: None\n");
        }
        AC_PRINTF("\n");
    }

    AC_FCLOSE(f);
    return ASTRAC_OK;
}
