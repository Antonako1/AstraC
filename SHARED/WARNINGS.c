/* Return TRUE when a diagnostic of the given severity should be emitted.
 *
 *   warning_level 0  -> no warnings (only errors)
 *   warning_level 1  -> standard warnings
 *   warning_level 2  -> all warnings
 *   warnings_as_errors -> treat every warning as an error (TRUE)
 *
 * The comparison is `warning_level <= args->warning_level`, so a level-1
 * diagnostic is emitted at warning level 1 or 2, and a level-2 diagnostic
 * only at warning level 2.
 */
BOOLEAN WARNING(U8 warning_level) {
    ASTRAC_ARGS *args = GET_ARGS();
    if (!args) return FALSE;
    if (args->warnings_as_errors) return TRUE;
    return (warning_level <= args->warning_level);
}