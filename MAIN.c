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

VOID PRINT_HELP() {
    AC_PRINTF("\n%s v%s\n\n", TRADEMARK, VERSION);
    AC_PRINTF(
        "ASTRAC.EXE [options] [flags]\n"

        "Options:\n"
            "  asm <file.AS>                    ; Assemble input file\n"
            "  comp <file.AC>                   ; Compile input file\n"
            "  disasm <file.BIN>                ; Disassemble input file\n"
            "  preproc <file.AC|file.AS>        ; Preprocess file\n"
            "  version                          ; Show version information\n"
            "  help                             ; Show this help message\n"
        
        "Flags:\n"
            "  macro <name> <value>             ; Define a macro for preprocessing\n"
            "  stepoff <level>                  ; Levels: 1=After preprocessing, 2=After assembling 3=After compiling\n"
            "  verbose                          ; Verbose output\n"

            "  arch <architecture>              ; Specify target architecture: i386 or i286. Default=i386\n"
            "  exe                              ; Specify to output a binary file with a simple header. Off by default.\n"
            "  lib                              ; Specify to output a binary file with a simple header. Off by default.\n"
            "  bits <16|32>                     ; Force 16-bit or 32-bit instruction encoding\n"
            "  org <address>                    ; Specify memory origin address for raw binaries (e.g., 0x7C00)\n"
            "  entry <label>                    ; Define the entry point for executables\n"
            "  warn <level>                     ; Warning level (0=none, 1=standard, 2=all, err=treat as errors)\n"
    );
}

VOID PRINT_VERSION() {
    AC_PRINTF("%s v%s\n", TRADEMARK, VERSION);
}

ASTRAC_RESULT START_WORKLOAD() {
    if (args.build_type == BUILD_TYPE_NONE) {
        AC_PRINTF("[ASTRAC] Error: no build mode selected (use asm, comp, disasm, or preproc)\n");
        return ASTRAC_ERR_ARGS;
    }
    switch (args.build_type) {
        case BUILD_TYPE_DISASSEMBLE:    return START_DISSASEMBLER();
        case BUILD_TYPE_PREPROCESS_ONLY: return ASTRAC_OK;
        case BUILD_TYPE_COMPILE:       return (ASTRAC_RESULT)START_COMPILER();
        case BUILD_TYPE_ASSEMBLE:       return START_ASSEMBLING();
        default:
            AC_PRINTF("[ASTRAC] Error: unknown build mode 0x%X\n", args.build_type);
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
                AC_PRINTF("[ASTRAC] Error: assemble requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } else if(ARG_CMP1("comp")) {
            args.build_type = BUILD_TYPE_COMPILE;
            if(i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: compile requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } else if(ARG_CMP1("disasm")) {
            args.build_type = BUILD_TYPE_DISASSEMBLE;
            if(i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: disasm requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } else if(ARG_CMP1("preproc")) {
            args.build_type = BUILD_TYPE_PREPROCESS_ONLY;
            if(i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: preproc requires an input file argument.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.input_file = argv[++i];
        } 
        
        
        else if(ARG_CMP1("macro")) {
            if (i + 2 >= argc) {
                AC_PRINTF("[ASTRAC] Error: macro requires two arguments: name and value.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 name = argv[++i];
            PU8 value = argv[++i];
            if (!DEFINE_MACRO(name, value, &args.macros)) {
                AC_PRINTF("[ASTRAC] Error: failed to define macro '%s'.\n", name);
                return ASTRAC_ERR_INTERNAL;
            }
        } 
        
        else if(ARG_CMP1("stepoff")) {
            if (i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: stepoff requires one argument: level.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 level_str = argv[++i];
            U32 level = 0;
            if (!AC_ATOI_E(level_str, &level) || level < 1 || level > 3) {
                AC_PRINTF("[ASTRAC] Error: invalid stepoff level '%s'. Must be 1, 2, or 3.\n", level_str);
                return ASTRAC_ERR_ARGS;
            }
            args.stepoff_level = (U8)level;
        } else if(ARG_CMP1("verbose")) {
            args.verbose = TRUE;
        } 
        
        else if(ARG_CMP1("arch")) {
            if(i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: arch requires one argument: architecture.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 arch_str = argv[++i];
            if(AC_STRCMP(arch_str, "i386") == 0) {
                args.arch = ARCH_I386;
            } else if(AC_STRCMP(arch_str, "i286") == 0) {
                args.arch = ARCH_I286;
            } else {
                AC_PRINTF("[ASTRAC] Error: unknown architecture '%s'. Supported: i386, i286.\n", arch_str);
                return ASTRAC_ERR_ARGS;
            }
        }
        else if(ARG_CMP1("exe")) {
            args.output_type = OUTPUT_EXE;
        }
        else if(ARG_CMP1("lib")) {
            args.output_type = OUTPUT_LIB;
        }
        else if(ARG_CMP1("bits")) {
            if(i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: bits requires one argument: 16 or 32.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 bits_str = argv[++i];
            if(AC_STRCMP(bits_str, "16") == 0) {
                args.dsm_bits = 16;
            } else if(AC_STRCMP(bits_str, "32") == 0) {
                args.dsm_bits = 32;
            } else {
                AC_PRINTF("[ASTRAC] Error: invalid bits value '%s'. Must be 16 or 32.\n", bits_str);
                return ASTRAC_ERR_ARGS;
            }
        }
        else if(ARG_CMP1("org")) {
            if(i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: org requires one argument: address.\n");
                return ASTRAC_ERR_ARGS;
            }
            PU8 addr_str = argv[++i];
            U32 addr = 0;
            if (!AC_ATOI_HEX_E(addr_str, &addr)) {
                AC_PRINTF("[ASTRAC] Error: invalid org address '%s'. Must be a hexadecimal number.\n", addr_str);
                return ASTRAC_ERR_ARGS;
            }
            args.org = addr;
        }
        else if(ARG_CMP1("entry")) {
            if(i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: entry requires one argument: label.\n");
                return ASTRAC_ERR_ARGS;
            }
            args.entry_point = argv[++i];
        }
        else if(ARG_CMP1("warn")) {
            if(i + 1 >= argc) {
                AC_PRINTF("[ASTRAC] Error: warn requires one argument: level.\n");
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
                AC_PRINTF("[ASTRAC] Error: invalid warn level '%s'. Must be 0, 1, 2, or 'err'.\n", level_str);
                return ASTRAC_ERR_ARGS;
            }
        }
        else {
            AC_PRINTF("[ASTRAC] Error: unrecognized argument '%s'.\n", arg);
        }
    }

    if (args.build_type == BUILD_TYPE_NONE) {
        AC_PRINTF("[ASTRAC] Error: no build mode selected (use asm, comp, disasm, or preproc)\n");
        return ASTRAC_ERR_ARGS;
    }
    
    if (!args.input_file) {
        AC_PRINTF("[ASTRAC] Error: no input file specified.\n");
        return ASTRAC_ERR_ARGS;
    }
    // create output file name if not specified
    if (!args.outfile) {
        args.outfile = AC_MAlloc(AC_STRLEN(args.input_file) + 5); // +5 for ".bin" and null terminator
        if (!args.outfile) {
            AC_PRINTF("[ASTRAC] Error: failed to allocate memory for output file name.\n");
            return ASTRAC_ERR_INTERNAL;
        }
        AC_STRCPY(args.outfile, args.input_file);
        PU8 dot = AC_STRRCHR(args.outfile, '.');
        if (dot) {
            *dot = '\0'; // remove existing extension
        }
        AC_STRCAT(args.outfile, ".BIN");
    }
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
