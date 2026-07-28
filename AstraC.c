/*
 * AstraC.c — Unity build entry point.
 *
 * This single translation unit compiles the entire AstraC toolchain.
 * Build with: cmake (root CMakeLists.txt stays unchanged).
 *
 * Layer structure:
 *   STDLIB/        — shell over cstdlib  (swap for your OS here)
 *   SHARED/        — preprocessor (shared between assembler and compiler)
 *   ASSEMBLER/     — x86 assembler pipeline
 *   COMPILER/      — AC language compiler pipeline
 *   DISSASEMBLER/  — binary disassembler
 *
 * To port to your OS: replace STDLIB/*.c with your kernel's implementations.
 * Licensed under the MIT License.
 */
#define _CRT_SECURE_NO_WARNINGS
#include "AstraC.h"

/* ══════════════════════════════════════════════════════════════════════
 *  STDLIB implementations  (swap this directory for your OS)
 * ══════════════════════════════════════════════════════════════════════ */
#include "STDLIB/IO.c"
#include "STDLIB/MEM.c"
#include "STDLIB/STRING.c"
#include "STDLIB/DEBUG.c"
#include "STDLIB/FS_DISK.c"
#include "STDLIB/BINARY.c"

/* ══════════════════════════════════════════════════════════════════════
 *  Shared pipeline stage: preprocessor
 * ══════════════════════════════════════════════════════════════════════ */
#include "SHARED/PREPROCESS.c"

/* ══════════════════════════════════════════════════════════════════════
 *  Assembler pipeline
 * ══════════════════════════════════════════════════════════════════════ */
#include "ASSEMBLER/AMAIN.c"
#include "ASSEMBLER/LEXER.c"
#include "ASSEMBLER/AST.c"
#include "ASSEMBLER/VERIFY_AST.c"
#include "ASSEMBLER/OPTIMIZE.c"
#include "ASSEMBLER/GEN.c"

/* ══════════════════════════════════════════════════════════════════════
 *  Disassembler
 * ══════════════════════════════════════════════════════════════════════ */
#include "DISSASEMBLER/DISMAIN.c"

/* ══════════════════════════════════════════════════════════════════════
 *  Compiler pipeline
 * ══════════════════════════════════════════════════════════════════════ */
#include "COMPILER/AST.c"
#include "COMPILER/LEX.c"
#include "COMPILER/PARSER.c"
#include "COMPILER/VERIFY_AST.c"
#include "COMPILER/GEN.c"
#include "COMPILER/COMPMAIN.c"

/* ══════════════════════════════════════════════════════════════════════
 *  Top-level: full-build orchestrator + argument parsing / main()
 * ══════════════════════════════════════════════════════════════════════ */
#include "FULL_BUILD.c"
#include "MAIN.c"
#include "SHARED/WARNINGS.c"

