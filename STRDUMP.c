/*
 * STRDUMP.c — ACFH binary string inspection tool.
 *
 * Dumps all null-terminated string literals from the .rodata section
 * of ACFH-headered binary executables/libraries.
 */
#include "AstraC.h"
#include "AC_FH.h"

STATIC VOID STRDUMP_PRINT_ESCAPED(PU8 str, U32 len) {
    for (U32 i = 0; i < len; i++) {
        U8 c = str[i];
        switch (c) {
            case '\n': AC_PRINTF("\\n"); break;
            case '\r': AC_PRINTF("\\r"); break;
            case '\t': AC_PRINTF("\\t"); break;
            case '\\': AC_PRINTF("\\\\"); break;
            case '\"': AC_PRINTF("\\\""); break;
            default:
                if (c >= 32 && c <= 126)
                    AC_PRINTF("%c", c);
                else
                    AC_PRINTF("\\x%02X", c);
                break;
        }
    }
}

ASTRAC_RESULT START_STRDUMP(VOID) {
    ASTRAC_ARGS *cfg = GET_ARGS();
    if (!cfg || !cfg->input_file) {
        AC_PRINTF_ERR("[STRDUMP] Error: no input file specified.\n");
        return ASTRAC_ERR_ARGS;
    }

    FILE *f = AC_FOPEN(cfg->input_file, MODE_FR);
    if (!f) {
        AC_PRINTF_ERR("[STRDUMP] Error: failed to open file '%s'\n", cfg->input_file);
        return ASTRAC_ERR_INTERNAL;
    }

    AC_FSEEK(f, 0, SEEK_END);
    U32 fsize = AC_FSIZE(f);
    AC_FSEEK(f, 0, SEEK_SET);

    AC_PRINTF("\n=== AstraC String Dump (.rodata) ===\n");
    AC_PRINTF("File: %s\n", cfg->input_file);
    AC_PRINTF("File size: %u bytes (0x%X)\n\n", fsize, fsize);

    if (fsize < sizeof(AC_FILE_HEADER)) {
        AC_PRINTF_ERR("[STRDUMP] Error: File is smaller than ACFH header (%u < %u bytes).\n",
                      fsize, (U32)sizeof(AC_FILE_HEADER));
        AC_FCLOSE(f);
        return ASTRAC_ERR_VERIFY;
    }

    AC_FILE_HEADER hdr;
    AC_MEMZERO(&hdr, sizeof(AC_FILE_HEADER));
    if (AC_FREAD(f, (U8*)&hdr, sizeof(AC_FILE_HEADER)) != sizeof(AC_FILE_HEADER)) {
        AC_PRINTF_ERR("[STRDUMP] Error: failed to read ACFH header.\n");
        AC_FCLOSE(f);
        return ASTRAC_ERR_INTERNAL;
    }

    /* Check magic */
    if (AC_MEMCMP(hdr.magic, AC_FILE_MAGIC, AC_FILE_MAGIC_LEN) != 0) {
        AC_PRINTF_ERR("[STRDUMP] Error: Invalid magic '%.4s' (expected '%s'). File is not a valid ACFH binary.\n",
                      hdr.magic, AC_FILE_MAGIC);
        AC_FCLOSE(f);
        return ASTRAC_ERR_VERIFY;
    }

    if (hdr.rodata_offset == OFFSET_NON_EXISTENT || hdr.rodata_size == 0 || hdr.rodata_offset == 0) {
        AC_PRINTF("[STRDUMP] Info: No .rodata section present in binary or .rodata size is 0.\n\n");
        AC_FCLOSE(f);
        return ASTRAC_OK;
    }

    if (hdr.rodata_offset + hdr.rodata_size > fsize) {
        AC_PRINTF_ERR("[STRDUMP] Error: .rodata section (0x%08X .. 0x%08X) extends beyond file size (0x%08X).\n",
                      hdr.rodata_offset, hdr.rodata_offset + hdr.rodata_size, fsize);
        AC_FCLOSE(f);
        return ASTRAC_ERR_VERIFY;
    }

    PU8 buf = (PU8)AC_MAlloc(hdr.rodata_size + 1);
    if (!buf) {
        AC_PRINTF_ERR("[STRDUMP] Error: failed to allocate memory for .rodata buffer.\n");
        AC_FCLOSE(f);
        return ASTRAC_ERR_INTERNAL;
    }

    AC_FSEEK(f, (long)hdr.rodata_offset, SEEK_SET);
    if (AC_FREAD(f, buf, hdr.rodata_size) != hdr.rodata_size) {
        AC_PRINTF_ERR("[STRDUMP] Error: failed to read .rodata section.\n");
        AC_MFree(buf);
        AC_FCLOSE(f);
        return ASTRAC_ERR_INTERNAL;
    }
    buf[hdr.rodata_size] = '\0';
    AC_FCLOSE(f);

    AC_PRINTF("-- .rodata Section Strings ---------------------------------\n");
    AC_PRINTF(".rodata Offset: 0x%08X\n", hdr.rodata_offset);
    AC_PRINTF(".rodata Size:   %u bytes (0x%X)\n\n", hdr.rodata_size, hdr.rodata_size);
    AC_PRINTF("  Idx     .rodata Offset   File Offset   Length   String\n");

    U32 str_count = 0;
    U32 i = 0;
    while (i < hdr.rodata_size) {
        /* Skip leading null bytes */
        while (i < hdr.rodata_size && buf[i] == '\0') i++;
        if (i >= hdr.rodata_size) break;

        U32 start_off = i;
        while (i < hdr.rodata_size && buf[i] != '\0') i++;
        U32 str_len = i - start_off;

        if (str_len > 0) {
            U32 file_off = hdr.rodata_offset + start_off;
            AC_PRINTF("  [%04u]  0x%08X       0x%08X    %6u   \"", str_count++, start_off, file_off, str_len);
            STRDUMP_PRINT_ESCAPED(&buf[start_off], str_len);
            AC_PRINTF("\"\n");
        }
        if (i < hdr.rodata_size && buf[i] == '\0') i++;
    }

    if (str_count == 0) {
        AC_PRINTF("  (No printable strings found in .rodata)\n");
    }

    AC_PRINTF("\nTotal strings dumped: %u\n\n", str_count);
    AC_MFree(buf);
    return ASTRAC_OK;
}
