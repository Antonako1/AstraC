ASTRAC_RESULT GET_MNEMONIC_INFO(PU8 mnemonic) {
    // Placeholder for actual mnemonic information retrieval logic
    // In a real implementation, this function would look up the mnemonic in a database or data structure
    // and print relevant information about it.
    
    // For demonstration purposes, we'll just print a mock response.
    U32 indexes[32] = {0}; // Placeholder for indexes
    U32 index_count = 0; // Placeholder for index count

    for(U32 i = 0; i < MNEM_COUNT; i++) {
        if(AC_STRICMP(mnemonic, asm_mnemonics[i].name) == 0) {
            indexes[index_count++] = i;
        }
    }

    if(index_count == 0) {
        AC_PRINTF("[ASTRAC] Error: mnemonic '%s' not found.\n", mnemonic);
        return ASTRAC_ERR_INTERNAL;
    }

    AC_PRINTF("[ASTRAC] Found %u entries for mnemonic '%s':\n", index_count, mnemonic);
    for(U32 i = 0; i < index_count; i++) {
        U32 idx = indexes[i];
        const ASM_MNEMONIC_TABLE *entry = &asm_mnemonics[idx];
        // Print all information about the mnemonic entry
        AC_PRINTF("Mnemonic: %s\n", entry->name);
        AC_PRINTF("  Prefix: 0x%02X\n", entry->prefix);
        AC_PRINTF("  Opcode: 0x%02X 0x%02X\n", entry->opcode[0], entry->opcode[1]);
        AC_PRINTF("  Encoding: %d\n", entry->encoding);
        AC_PRINTF("  Operand Types: %d, %d, %d, %d\n", entry->operand[0], entry->operand[1], entry->operand[2], entry->operand[3]);
        AC_PRINTF("  Operand Count: %d\n", entry->operand_count);
        AC_PRINTF("  Size: %d\n", entry->size);
        AC_PRINTF("  Description: %s\n", entry->description);
        AC_PRINTF("\n");
    }

    return ASTRAC_OK;
}

ASTRAC_RESULT START_MNEMONIC_INFO(PU8 mnemonic) {
    ASTRAC_RESULT res = ASTRAC_OK;
    AC_PRINTF("[ASTRAC] Starting mnemonic info for: '%s'\n", mnemonic);
    
    // Call the function to get mnemonic information
    res = GET_MNEMONIC_INFO(mnemonic);
    
    if (res != ASTRAC_OK) {
        AC_PRINTF("[ASTRAC] Error: Failed to retrieve mnemonic info for '%s'.\n", mnemonic);
    }
    
    return res;
}