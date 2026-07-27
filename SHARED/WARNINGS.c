BOOLEAN WARNING(U8 warning_level) {
    ASTRAC_ARGS   *args = GET_ARGS();
    if(args->warnings_as_errors) {
        AC_PRINTF("[ASTRAC] Error: treating warnings as errors\n");
        return TRUE;
    }
    if (warning_level <= args->warning_level) {
        if (args->warnings_as_errors) {
            AC_PRINTF("[ASTRAC] Error: treating warnings as errors\n");
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}