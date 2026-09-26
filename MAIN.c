/*
 * MAIN.c — Argument parsing, dispatch, and program entry point (skeleton).
 *
 * Parses command-line arguments into ASTRAC_ARGS, then calls START_WORKLOAD()
 * which dispatches to the correct pipeline stage.
 */
#include "AstraC.h"
#include "ASSEMBLER/ASSEMBLER.h"
#include "DISSASEMBLER/DISSASEMBLER.h"

#define ARG_CMP1(x)    (AC_STRICMP(arg, x) == 0)
#define ARG_CMP2(x, y) (ARG_CMP1(x) || ARG_CMP1(y))

static ASTRAC_ARGS args ATTRIB_DATA = { 0 };

ASTRAC_ARGS *GET_ARGS() { return &args; }

/*
 * START_SHOWLINE — print a window of lines from a preprocessed temp file.
 *
 *   showline AS 5 10     -> lines 5..15 of 00.AS
 *   showline AS 5 10 20  -> lines 5..25 of 00.AS
 *
 * Output format: "{line number} : {content}".
 */
STATIC ASTRAC_RESULT START_SHOWLINE() {
    #ifdef _WIN32
    PU8 path = args.showline_is_ac ? (PU8)"C:\\TMP\\00.AC" : (PU8)"C:\\TMP\\00.AS";
    #else
    PU8 path = args.showline_is_ac ? (PU8)"/tmp/00.AC" : (PU8)"/tmp/00.AS";
    #endif

    FILE *f = AC_FOPEN(path, MODE_R | MODE_FAT32);
    if (!f) {
        AC_PRINTF_ERR("[SHOWLINE] Cannot open file: %s\n", path);
        return ASTRAC_ERR_INTERNAL;
    }

    U32 ctx   = args.showline_ctx;
    U32 start = args.showline_start;
    U32 from  = (start > ctx) ? (start - ctx) : 1;
    U32 to    = args.showline_has_end ? (args.showline_end + ctx)
                                      : (start + ctx);

    U32 cur  = 1;
    U8  line[BUF_SZ];
    while (AC_FILE_GET_LINE(f, line, sizeof(line))) {
        if (cur >= from && cur <= to) {
            U32 len = (U32)AC_STRLEN(line);
            while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
                line[--len] = '\0';
            AC_PRINTF("%u : %s\n", cur, line);
        }
        cur++;
        if (cur > to) break;
    }

    AC_FCLOSE(f);
    return ASTRAC_OK;
}

VOID PRINT_HELP() {
    AC_PRINTF("\n%s v%s (%s)\n\n", TRADEMARK, VERSION, PLATFORM);
    AC_PRINTF(
        "ASTRAC.EXE [options] [flags]\n"

        "Options:\n"
            "  asm <file.AS>                    ; Assemble input file\n"
            "  comp <file.AC>                   ; Compile input file\n"
            "  disasm <file.BIN>                ; Disassemble input file\n"
            "  objdump <file.BIN> [mode]        ; Dump ACFH header and tables (all|header|tables|funcs|relocs)\n"
            "  strdump <file.BIN>               ; Dump strings from ACFH binary rodata section\n"
            "  preproc <file.AC|file.AS>        ; Preprocess file\n"
            "  info <mnemonic>                  ; Show information about a mnemonic\n"
            "  showline <AS|AC> <ctx> <start> [end] ; Show source lines around a line number\n"
            "  version                          ; Show version information\n"
            "  help                             ; Show this help message\n"
        
        "Flags:\n"
            "  macro <name> <value>             ; Define a macro for preprocessing\n"
            "  stepoff <level>                  ; Levels: 1=After preprocessing, 2=After assembling 3=After compiling\n"
            "  verbose                          ; Verbose output\n"
            "  debug                            ; Debug output to files. (AC->AS, AS->ASD)\n"
            "  arch <architecture>              ; Specify target architecture: i386 or i286. Default=i386\n"
            "  type <exe|lib> [tables...]       ; Binary output format (exe/lib) with optional tables (offset_table|ot, function_table|ft)\n"
            "  exe                              ; Specify to output a binary file with an executable ACFH header\n"
            "  lib                              ; Specify to output a binary file with a library ACFH header\n"
            "  offset_table / ot                ; Emit relocation offset table in ACFH header\n"
            "  function_table / ft              ; Emit function export table in ACFH header\n"
            "  bits <16|32>                     ; Force 16-bit or 32-bit instruction encoding\n"
            "  org <address>                    ; Specify memory origin address for raw binaries (e.g., 0x7C00)\n"
            "  entry <label>                    ; Define the entry point for executables\n"
            "  warn <level>                     ; Warning level (0=none, 1=standard, 2=all, err=treat as errors)\n"
            "  debug                           ; Emit source-line comments in generated .AS for debugging\n"
    );
}

VOID PRINT_VERSION() {
    AC_PRINTF("%s v%s (%s)\n", TRADEMARK, VERSION, PLATFORM);
}

ASTRAC_RESULT START_WORKLOAD() {
    if (args.build_type == BUILD_TYPE_NONE) {
        AC_PRINTF_ERR("[ASTRAC] Error: no build mode selected (use asm, comp, disasm, objdump, or preproc)\n");
        return ASTRAC_ERR_ARGS;
    }
    switch (args.build_type) {
        case BUILD_TYPE_DISASSEMBLE:    return START_DISSASEMBLER();
        case BUILD_TYPE_OBJDUMP:        return START_OBJDUMP();
        case BUILD_TYPE_STRDUMP:        return START_STRDUMP();
        case BUILD_TYPE_PREPROCESS_ONLY: return ASTRAC_OK;
        case BUILD_TYPE_COMPILE:       return (ASTRAC_RESULT)START_COMPILER();
        case BUILD_TYPE_ASSEMBLE:       return START_ASSEMBLING();
        default:
            AC_PRINTF_ERR("[ASTRAC] Error: unknown build mode 0x%X\n", args.build_type);
            return ASTRAC_ERR_INTERNAL;
    }
}

VOID FREE_ARGS() {
    FREE_MACROS(&args.macros);
    AC_MFree(args.outfile);
    AC_MEMSET(&args, 0, sizeof(ASTRAC_ARGS));
}

U32 main(U32 argc, PPU8 argv) {
    if (argc < 2) {
        PRINT_HELP();
        return ASTRAC_ERR_ARGS;
    }

    AC_PRINTF("\n%s\n", TRADEMARK);
    AC_MEMZERO(&args, sizeof(ASTRAC_ARGS));

    for (U32 i = 1; i < argc; i++) {
        PU8 arg = argv[i];
        if(ARG_CMP2("help", "-h")) {
            PRINT_HELP();
            return ASTRAC_OK;
        } else if(ARG_CMP2("version", "-v")) {
            PRINT_VERSION();
            return ASTRAC_OK;
        } else if(ARG_CMP1("asm")) {
            args.build_type = BUILD_TYPE_ASSEMBLE;
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: assemble requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } else if(ARG_CMP1("comp")) {
            args.build_type = BUILD_TYPE_COMPILE;
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: compile requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } else if(ARG_CMP1("disasm")) {
            args.build_type = BUILD_TYPE_DISASSEMBLE;
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: disasm requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } else if(ARG_CMP1("objdump")) {
            args.build_type = BUILD_TYPE_OBJDUMP;
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: objdump requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
            if (i + 1 < argc) {
                PU8 sub = argv[i + 1];
                if (AC_STRICMP(sub, "all") == 0) { args.objdump_mode = OBJDUMP_MODE_ALL; i++; }
                else if (AC_STRICMP(sub, "header") == 0 || AC_STRICMP(sub, "hdr") == 0 || AC_STRICMP(sub, "info") == 0) { args.objdump_mode = OBJDUMP_MODE_HEADER; i++; }
                else if (AC_STRICMP(sub, "tables") == 0 || AC_STRICMP(sub, "tbl") == 0) { args.objdump_mode = OBJDUMP_MODE_TABLES; i++; }
                else if (AC_STRICMP(sub, "funcs") == 0 || AC_STRICMP(sub, "ft") == 0 || AC_STRICMP(sub, "function_table") == 0) { args.objdump_mode = OBJDUMP_MODE_FUNCS; i++; }
                else if (AC_STRICMP(sub, "relocs") == 0 || AC_STRICMP(sub, "ot") == 0 || AC_STRICMP(sub, "offset_table") == 0) { args.objdump_mode = OBJDUMP_MODE_RELOCS; i++; }
            }
        } else if(ARG_CMP1("strdump")) {
            args.build_type = BUILD_TYPE_STRDUMP;
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: strdump requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } else if(ARG_CMP1("preproc")) {
            args.build_type = BUILD_TYPE_PREPROCESS_ONLY;
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: preproc requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } 
        else if(ARG_CMP1("info")) {
            args.build_type = BUILD_TYPE_MNEMONIC_INFO;
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: info requires a mnemonic argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        }
        else if(ARG_CMP1("showline")) {
            args.build_type = BUILD_TYPE_SHOWLINE;
            if (i + 3 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: showline requires: {AS|AC} {context} {start line} [end line]\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 kind = argv[++i];
            if (AC_STRICMP(kind, "AC") == 0)      args.showline_is_ac = TRUE;
            else if (AC_STRICMP(kind, "AS") == 0) args.showline_is_ac = FALSE;
            else {
                AC_PRINTF_ERR("[ASTRAC] Error: showline file kind must be 'AS' or 'AC' (got '%s').\n", kind);
                return ASTRAC_ERR_ARGS;
            }
            PU8 ctx_str = argv[++i];
            if (!AC_ATOI_E(ctx_str, &args.showline_ctx)) {
                AC_PRINTF_ERR("[ASTRAC] Error: invalid showline context '%s'.\n", ctx_str);
                return ASTRAC_ERR_ARGS;
            }
            PU8 start_str = argv[++i];
            if (!AC_ATOI_E(start_str, &args.showline_start)) {
                AC_PRINTF_ERR("[ASTRAC] Error: invalid showline start line '%s'.\n", start_str);
                return ASTRAC_ERR_ARGS;
            }
            args.showline_has_end = FALSE;
            args.showline_end     = 0;
            if (i + 1 < argc) {
                U32 end_val = 0;
                if (AC_ATOI_E(argv[i + 1], &end_val)) {
                    args.showline_end     = end_val;
                    args.showline_has_end = TRUE;
                    i++;
                }
            }
        }
        
        
        else if(ARG_CMP1("macro")) {
            if (i + 2 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: macro requires two arguments: name and value.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 name = argv[++i];
            PU8 value = argv[++i];
            if (!DEFINE_MACRO(name, value, &args.macros)) {
                AC_PRINTF_ERR("[ASTRAC] Error: failed to define macro '%s'.\n", name);
                return ASTRAC_ERR_INTERNAL;
            }
        } 
        
        else if(ARG_CMP1("stepoff")) {
            if (i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: stepoff requires one argument: level.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 level_str = argv[++i];
            U32 level = 0;
            if (!AC_ATOI_E(level_str, &level) || level < 1 || level > 3) {
                AC_PRINTF_ERR("[ASTRAC] Error: invalid stepoff level '%s'. Must be 1, 2, or 3.\n", level_str);
                return ASTRAC_ERR_ARGS;
            }
            args.stepoff_level = (U8)level;
        } else if(ARG_CMP1("verbose")) {
            args.verbose = TRUE;
        } 
        
        else if(ARG_CMP1("arch")) {
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: arch requires one argument: architecture.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 arch_str = argv[++i];
            if(AC_STRCMP(arch_str, "i386") == 0) {
                args.arch = ARCH_I386;
            } else if(AC_STRCMP(arch_str, "i286") == 0) {
                args.arch = ARCH_I286;
            } else {
                AC_PRINTF_ERR("[ASTRAC] Error: unknown architecture '%s'. Supported: i386, i286.\n", arch_str);
                return ASTRAC_ERR_ARGS;
            }
        }
        else if(ARG_CMP1("type")) {
            if (i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: type requires 'exe' or 'lib'.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 target = argv[++i];
            if (AC_STRICMP(target, "exe") == 0) {
                args.output_type = OUTPUT_EXE;
            } else if (AC_STRICMP(target, "lib") == 0) {
                args.output_type = OUTPUT_LIB;
            } else {
                AC_PRINTF_ERR("[ASTRAC] Error: invalid binary type '%s' (expected exe or lib).\n", target);
                return ASTRAC_ERR_ARGS;
            }
            while (i + 1 < argc) {
                PU8 next_arg = argv[i + 1];
                if (AC_STRICMP(next_arg, "offset_table") == 0 || AC_STRICMP(next_arg, "ot") == 0) {
                    args.emit_offset_table = TRUE;
                    i++;
                } else if (AC_STRICMP(next_arg, "function_table") == 0 || AC_STRICMP(next_arg, "ft") == 0) {
                    args.emit_func_table = TRUE;
                    i++;
                } else {
                    break;
                }
            }
        }
        else if(ARG_CMP1("exe")) {
            args.output_type = OUTPUT_EXE;
        }
        else if(ARG_CMP1("lib")) {
            args.output_type = OUTPUT_LIB;
        }
        else if(ARG_CMP1("offset_table") || ARG_CMP1("ot")) {
            args.emit_offset_table = TRUE;
        }
        else if(ARG_CMP1("function_table") || ARG_CMP1("ft")) {
            args.emit_func_table = TRUE;
        }
        else if(ARG_CMP1("bits")) {
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: bits requires one argument: 16 or 32.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 bits_str = argv[++i];
            if(AC_STRCMP(bits_str, "16") == 0) {
                args.dsm_bits = 16;
            } else if(AC_STRCMP(bits_str, "32") == 0) {
                args.dsm_bits = 32;
            } else {
                AC_PRINTF_ERR("[ASTRAC] Error: invalid bits value '%s'. Must be 16 or 32.\n", bits_str);
                return ASTRAC_ERR_ARGS;
            }
        }
        else if(ARG_CMP1("org")) {
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: org requires one argument: address.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 addr_str = argv[++i];
            U32 addr = 0;
            if (!AC_ATOI_HEX_E(addr_str, &addr)) {
                AC_PRINTF_ERR("[ASTRAC] Error: invalid org address '%s'. Must be a hexadecimal number.\n", addr_str);
                return ASTRAC_ERR_ARGS;
            }
            args.org = addr;
        }
        else if(ARG_CMP1("entry")) {
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: entry requires one argument: label.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.entry_point = argv[++i];
        }
        else if(ARG_CMP1("debug")) {
            args.debug = TRUE;
        }
        else if(ARG_CMP1("warn")) {
            if(i + 1 >= argc) {
                AC_PRINTF_ERR("[ASTRAC] Error: warn requires one argument: level.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 level_str = argv[++i];
            if(AC_STRCMP(level_str, "0") == 0) {
                args.warning_level = 0;
            } else if(AC_STRCMP(level_str, "1") == 0) {
                args.warning_level = 1;
            } else if(AC_STRCMP(level_str, "2") == 0) {
                args.warning_level = 2;
            } else if(AC_STRCMP(level_str, "err") == 0) {
                args.warnings_as_errors = TRUE;
            } else {
                AC_PRINTF_ERR("[ASTRAC] Error: invalid warn level '%s'. Must be 0, 1, 2, or 'err'.\n", level_str);
                return ASTRAC_ERR_ARGS;
            }
        }
        else {
            AC_PRINTF_ERR("[ASTRAC] Error: unrecognized argument '%s'.\n", arg);
        }
    }

    if (args.build_type == BUILD_TYPE_NONE) {
        AC_PRINTF_ERR("[ASTRAC] Error: no build mode selected (use asm, comp, disasm, or preproc)\n");
        return ASTRAC_ERR_ARGS;
    }

    if (args.build_type == BUILD_TYPE_SHOWLINE) {
        ASTRAC_RESULT res = START_SHOWLINE();
        FREE_ARGS();
        return (U32)res;
    }
    
    if (!args.input_file) {
        AC_PRINTF_ERR("[ASTRAC] Error: no input file specified.\n");
        return ASTRAC_ERR_ARGS;
    }
    
    if(args.build_type == BUILD_TYPE_MNEMONIC_INFO) {
        AC_PRINTF("[ASTRAC] Info mode selected for mnemonic: '%s'\n", args.input_file);
        ASTRAC_RESULT res = START_MNEMONIC_INFO(args.input_file);
        FREE_ARGS();
        return (U32)res;
    }

    if(args.build_type == BUILD_TYPE_OBJDUMP) {
        ASTRAC_RESULT res = START_OBJDUMP();
        FREE_ARGS();
        return (U32)res;
    }

    if(args.build_type == BUILD_TYPE_STRDUMP) {
        ASTRAC_RESULT res = START_STRDUMP();
        FREE_ARGS();
        return (U32)res;
    }

    // create output file name if not specified
    if (!args.outfile) {
        args.outfile = AC_MAlloc(AC_STRLEN(args.input_file) + 5); // +5 for ".bin" and null terminator
        if (!args.outfile) {
            AC_PRINTF_ERR("[ASTRAC] Error: failed to allocate memory for output file name.\n");
            return ASTRAC_ERR_INTERNAL;
        }
        AC_STRCPY(args.outfile, args.input_file);
        PU8 dot = AC_STRRCHR(args.outfile, '.');
        if (dot) {
            *dot = '\0'; // remove existing extension
        }
        AC_STRCAT(args.outfile, ".BIN");
    }
    args.PARSER_TOPLEVEL_LOG_PUSH_TAIL = 0;
    args.PARSER_TOPLEVEL_LOG_POP_TAIL = 0;
    
    if (args.dsm_bits == 0) args.dsm_bits = 32;
    if (!args.entry_point) args.entry_point = "main";
    if(args.arch == ARCH_NONE) args.arch = ARCH_I386;
    PU8 arch_str = (args.arch == ARCH_I386) ? "i386" : (args.arch == ARCH_I286) ? "i286" : "unknown";
    PU8 build_type_str = (args.build_type == BUILD_TYPE_COMPILE) ? "COMPILE" :
                        (args.build_type == BUILD_TYPE_ASSEMBLE) ? "ASSEMBLE" :
                        (args.build_type == BUILD_TYPE_BUILD) ? "BUILD" :
                        (args.build_type == BUILD_TYPE_DISASSEMBLE) ? "DISASSEMBLE" :
                        (args.build_type == BUILD_TYPE_PREPROCESS_ONLY) ? "PREPROCESS_ONLY" : "UNKNOWN";
    AC_PRINTF("[ASTRAC] Build type: %s, \n\tinput: '%s', \n\toutput: '%s', \n\tarch: %s, \n\tbits: %u, \n\torg: 0x%X, \n\tentry: '%s'\n",
        build_type_str, args.input_file, args.outfile,
        arch_str,
        args.dsm_bits, args.org, args.entry_point);

    ASTRAC_RESULT res = START_WORKLOAD();
    FREE_ARGS();
    return (U32)res;
}
