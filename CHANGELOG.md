# Changelog

All notable changes to AstraC will be documented in this file.

## 2026-10-19

### Preprocessor `#line` Source Tracking & Per-Target Temporary Files
- Implemented automatic `#line <line> "<file>"` marker emission in `SHARED/PREPROCESS.c` across file entry, `#include` enter/return boundaries, and multi-line comments/skipped conditional blocks (`READ_LOGICAL_LINE_CNT`).
- Added explicit `#line <line> ["file"]` directive support (`IDENT_LINE`) in `SHARED/PREPROCESS.c`, `COMPILER/LEX.c`, and `ASSEMBLER/LEXER.c` so compiler and assembler diagnostics accurately report original source files and line numbers instead of preprocessed temporary paths.
- Replaced hardcoded global `/TMP/00.AC` and `/TMP/00.AS` temporary files with per-target `<base>.I` (AC compiler) and `<base>.ASI` (assembler) files to eliminate race conditions during parallel builds, automatically cleaning them up unless `stepoff 1`, `preproc`, or `debug` is specified.

### `.bss` Section Support & `ATTRIB_DATA` / `ATTRIB_RODATA` / `ATTRIB_BSS` Variable Attributes
- Added `ATTRIB_DATA`, `ATTRIB_RODATA`, and `ATTRIB_BSS` variable placement attributes to `STDLIB/TYPEDEF.h`, `COMPILER/COMPILER.h`, `COMPILER/LEX.c`, `COMPILER/PARSER.c`, and `COMPILER/GEN.c` (supported in both prefix and suffix positions on global variables and arrays).
- Defaulted uninitialized AC global variables and arrays to `.bss` (`SYM_SEC_BSS`) and initialized globals to `.data` (`SYM_SEC_DATA`), while honoring explicit `ATTRIB_DATA`, `ATTRIB_RODATA`, and `ATTRIB_BSS` overrides.
- Implemented `.bss` (`DIR_BSS`) section directive in `ASSEMBLER/LEXER.c`, `ASSEMBLER/AST.c`, `ASSEMBLER/VERIFY_AST.c`, and `ASSEMBLER/GEN.c` with **0 disk bytes** emitted. Computed ACFH table byte sizes (`COMPUTE_ACFH_TABLE_BYTES`) after Pass 1 so `.bss` virtual addresses sit after all trailing ACFH tables (`h.bss_offset` = end of file on disk, `h.bss_size = ptrs.bss`) for flat in-place OS loaders.

### Cross-File `local` Scoping & Separate Type vs. Variable/Function Namespaces
- Enforced file-private `local` visibility across `#include` files in `COMPILER/PARSER.c`, `COMPILER/VERIFY_AST.c`, and `COMPILER/GEN.c` using `#line`-derived `file_id`s, and scoped assembly labels (`SYM_ASM_NAME` -> `f<file_id>_<name>`) so multiple files can define same-named `local` globals or functions without assembler collisions.
- Separated type symbols (`SYM_STRUCT`, `SYM_UNION`, `SYM_ENUM`, `SYM_TYPEDEF`) from variable/function symbols (`SYM_VARIABLE`, `SYM_FUNCTION`, `SYM_ENUM_VAL`) via dedicated `SYM_LOOKUP_TYPE` / `V_FIND_TYPE_SYM` / `FIND_TYPE_SYM` lookups so case-insensitive declarations like `STUDENT student; STUDENT student2;` never shadow or overwrite the type definition.

### ACFH-Aware Disassembler (`disasm`) & Symbol/Import Annotation
- Upgraded `DISSASEMBLER/DISMAIN.c` to automatically detect `"ACFH"` headers at byte 0, skip the 112-byte header, and disassemble only the `.code` section (`code_offset..code_offset+code_size`).
- Added automatic symbol resolution in `disasm` using the ACFH Function Export Table (`ft`) and Import Table (`it`): emits `<func>:` headers before function entry points and annotates relative `CALL`/`JMP` instructions with `<func>` or `<lib::func>`.

### Dynamic `AC_ReAlloc` Table Growth & CLI Improvements
- Replaced fixed-capacity arrays across `COMPILER/` (`SYM_TABLE`, `rodata_strings`, `imports`, `files`) and `ASSEMBLER/` (`ASM_TOK_ARRAY`, `ASM_AST_ARRAY`, `LABEL_TABLE`, `ASM_PTR_ARRAY`, `ASM_IMPORT_TABLE`) with dynamically growing `AC_ReAlloc` buffers.
- Wired `preproc <file.AC|file.AS>` in `MAIN.c` to execute `PREPROCESS_ASM` / `PREPROCESS_C` and output `<base>.I` / `<base>.ASI`.
- Enhanced `showline <file|AS|AC> <ctx> <start> [end]` in `MAIN.c` to accept arbitrary source file paths in addition to legacy `AS`/`AC` aliases.
- Fixed CLI help output in `MAIN.c` (`stepoff` stage order description and duplicate `debug` line) and added compiler test suite `TESTS/COMPILER/15_toolchain_improvements.ac` (15/15 tests passing).

## 2026-10-12

### ACFH Header & Binary Code Section Offset Alignment
- Fixed binary code offset padding in `ASSEMBLER/GEN.c`: removed redundant `ptrs.code = sizeof(AC_FILE_HEADER)` initialization in Pass 1 and Pass 2. The code section now begins immediately following the 112-byte (`0x70`) header at file offset `0x70` instead of `0xE0`, eliminating 112 unwritten zero padding bytes that previously prevented jumping past the header (e.g. `jmp 0x10070` when `org` is `0x10000`).
- Fixed heap corruption crash (`0xC0000374`) on entry point string formatting in `ASSEMBLER/GEN.c`: replaced unsafe `AC_MAlloc(AC_STRLEN(cfg->entry_point + 2))` buffer overrun with a safe stack buffer `U8 entry[128]` with fallback lookup.
- Fixed Pass 2 import patch tracking in `ASSEMBLER/GEN.c`: reset `asm_imports.patch_count = 0` at the beginning of Pass 2 to avoid duplicate patch entries.

### Compiler Codegen Fixes (Struct Copy, Array Bases, Member Address-of)
- Fixed struct copy source evaluation in `COMPILER/GEN.c`: updated `GEN_IDENT` to recognize aggregate types (`IS_AGGREGATE_TYPE(s->type)`) and emit `LEA EAX, [EBP-offset]` instead of scalar `MOV EAX, [EBP-offset]`. When copying structs (`p2 = p1`), `EAX` now holds the struct base address for `GEN_STRUCT_COPY` instead of loading the first 4 bytes as a scalar pointer, resolving runtime memory faults.
- Fixed local array base address resolution in `COMPILER/GEN.c`: updated `GEN_ARR_BASE` to check stack array allocation (`s->array_size > 0 && (I32)s->offset <= 0`) before checking pointer depth, ensuring `LEA` is emitted for local arrays such as `PU8 arr[3]` rather than reading an uninitialized stack slot via `MOV`.
- Fixed address-of struct member codegen in `COMPILER/GEN.c`: updated `GEN_ADDR` to delegate directly to `GEN_LVALUE_ADDR(target)`, preventing an extraneous `MOV EAX, [EAX]` dereference when taking the address of struct fields (e.g. `&node.pt.weight`).

### CLI Hexadecimal Prefix Parsing
- Enhanced `org` flag argument parsing in `MAIN.c`: added automatic detection of `0x` and `0X` hexadecimal prefixes using `AC_ATOI_HEX_EP` while preserving backward compatibility with raw hex string parsing via `AC_ATOI_HEX_E`.

### OS Bootloader Second Stage Capacity Expansion
- Expanded BIOS `INT 0x13` floppy read sector count from 18 sectors (9 KB) to 32 sectors (16 KB) in `TESTS/OS/BOOTLOADER/SECOND_STAGE.AS`, preventing premature truncation of kernel binaries exceeding 9 KB.

## 2026-09-27

### Dynamic Import Table Call Site Patch Recording
- Fixed Import Table patch recording in `ASSEMBLER/GEN.c`: updated import tracking structures (`ASM_IMPORT_TABLE`, `ASM_IMPORT_PATCH`) so that every call site occurrence of an imported function (e.g. multiple `malloc` or `free` calls) generates a dedicated `AC_IMPORT_ENTRY` containing its specific `patch_offset`.
- Added string table pool deduplication (`GET_OR_ADD_STRTAB`) in `ASSEMBLER/GEN.c` so repeated library and function names share string pool entries while preserving accurate call site patch locations.

## 2026-09-26

### Relocation Table Generation Fixes
- Fixed relocation table output when requested via CLI flags (`type exe ot` / `type lib ot` / `offset_table`): `ASSEMBLER/GEN.c` now respects `cfg->emit_offset_table` when populating `h.flags` (`AC_FLAG_HAS_RELOCS`) and `h.reloc_offset`, ensuring relocation headers are emitted even when entry count is 0.
- Fixed missing relocation recording for data section symbol references: `ENCODE_DATA_VAR()` and `eval_atom()` in `ASSEMBLER/GEN.c` now call `RECORD_RELOC()` with section-cumulative offsets (`CURRENT_CUMULATIVE_OFFSET()`) when resolving symbol addresses in `.data` and `.rodata` initializers (such as `_ptr_var: DD _my_var`).
- Enhanced `OBJDUMP.c` relocation table inspector: updated header check to display relocation table metadata (offset and size) when `reloc_offset` is present, and added dynamic section relative offset formatting (`.code+0x...`, `.data+0x...`, `.rodata+0x...`).

### Variable Assignment Optimization & Nested Control Flow Test
- Optimized variable assignment lowering in `COMPILER/GEN.c`: eliminated redundant `PUSH EAX` / `POP EAX` stack instructions for simple variable assignments (`x = expr;`), storing `EAX` directly to variable memory.
- Added `test_nested_control_flow()` to `TESTS/COMPILER/14_comprehensive_features.ac` verifying that `break;` inside a `switch` statement enclosed within a `while` loop targets the switch exit label, not the enclosing loop.
- Updated `DOCS/AC_LANG.md` and website documentation (`docs/ac.php`) to clarify `F32` floating-point storage, IEEE-754 literal encoding, struct embedding, and parameter passing support.

### Pointer Arithmetic, Switch Control Flow, and F32 Float Fixes
- Fixed pointer arithmetic scaling in `COMPILER/GEN.c`: `ptr + i`, `i + ptr`, `ptr - i`, `ptr1 - ptr2`, `ptr += i`, and `ptr -= i` now scale integer offsets by `GEN_TYPE_SIZE(element_type)` via `IMUL` / `DIV` instructions.
- Fixed `switch` statement control flow & stack handling in `COMPILER/GEN.c`: pushed `switch_expr` onto execution stack once, used `MOV EAX, [ESP]` peeking for case comparisons, wired `lbl_end` to `ctx->loop_label_stack` for `break;` statements, and restored stack pointer `ADD ESP, 4` at switch exit.
- Fixed `F32` floating point literal lexing (`COMPILER/LEX.c`) and codegen (`COMPILER/GEN.c`): fractional digits after `.` are parsed into float values, and float literals emit 32-bit IEEE-754 binary representation (e.g. `0x3FC00000` for `1.5f`).
- Updated `TESTS/COMPILER/14_comprehensive_features.ac` with strict mathematical assertions (`r_complex == 3922`, `*(ptr+i)` checks, `0x3FC00000` bit check).
- Updated `TESTS/RUN_COMPILER_TESTS.ps1` with assembly assertions for scaled pointer arithmetic, switch stack peeking, and F32 binary representation.

### Comprehensive Feature Test Suite
- Added compiler test suite `TESTS/COMPILER/14_comprehensive_features.ac` integrating `foo(a,b)`, array indexing, pointer arithmetic, nested structs, function pointers, switch statements, `F32` floating point types, variadic function calls (`VA_LIST`, `va_start`, `va_arg`, `va_end`), complex nested expressions, register allocation, and loops (`while`, `for`).
- Updated `TESTS/RUN_COMPILER_TESTS.ps1` to verify `_FOO` symbol emission and clean compilation across all 14 test cases.

### Unsized Global Array Declarations & Initializer Lists
- Added support for unsized array declarations with initializer lists (e.g., `U32 g_arr[] = {1, 2, 3};`) in `COMPILER/PARSER.c`: empty brackets `[]` are allowed when followed by an initializer list, inferring `array_size` from `init_count`.
- Fixed assembler verification (`ASSEMBLER/VERIFY_AST.c`) for non-BYTE list variables (e.g. `DD 1, 2, 3`), skipping single-value `is_numeric_value` validation on list initializers.
- Updated `TESTS/COMPILER/01_types_and_vars.ac` to test unsized global array `U32 g_arr_2[] = {1, 2, 3};`.

### Double Pointer Verification & Test Suite
- Added compiler test suite `TESTS/COMPILER/13_double_pointers.ac` testing `U8**`, `PPU8`, double dereferencing (`**pp`), address-of (`&`), pointer arithmetic, and double indexing (`ptr_arr[i]`).
- Verified assembly generation (`MOV EAX, [EAX]` pointer dereference + `MOVZX EAX, BYTE [EAX]` target byte load, `MOV [EAX], BL` byte write).
- Updated `TESTS/RUN_COMPILER_TESTS.ps1` with specific assembly assertions verifying `MOVZX EAX, BYTE [EAX]` for double pointer dereferencing (all 13 compiler tests passing).

### CLI Help Flags & Verbose Table Output
- Added `-h` and `--help` CLI flag aliases alongside `help` for standard command line usage.
- Added `--version` and `-v` CLI flag aliases alongside `version`.
- Enhanced `verbose` output mode to log full itemized contents of Relocation Tables, Function Export Tables, and Import Tables during binary code generation.

### ACFH Import Tables & #import Directive
- Implemented ACFH Import Tables (`AC_IMPORT_TABLE_HDR`, `AC_IMPORT_ENTRY`, `AC_FLAG_HAS_IMPORTS`) in binary header structure (`AC_FH.h`) to support dynamic function resolution by OS loaders during multitasking. Preserved 108-byte header size (`reserved[40]`).
- Added `#import <funcs> from "lib.lib"` directive in preprocessor, AC lexer (`CTOK_KW_IMPORT`), parser, AST, and codegen, allowing AC source files to specify external library origins.
- Implemented `.import "lib.lib" FUNC1, FUNC2` directive support in assembler lexer, AST (`ADD_ASM_IMPORT`), verifier (`IS_ASM_IMPORTED_SYMBOL`), and binary codegen.
- Added `objdump <file.BIN> imports` (or `it` / `import_table`) sub-argument to display imported library names, symbol names, and patch offsets.
- Added compiler test file `TESTS/COMPILER/12_acfh_imports.ac` and updated `TESTS/RUN_COMPILER_TESTS.ps1` (all 12 tests passing).
- Updated preprocessor, assembler, and language documentation (`DOCS/PREPROCESSOR.md`, `DOCS/AC_LANG.md`, `DOCS/AS.md`, `DOCS/CLI.md`, `DOCS/AC_FILEHEADER.md`, and website pages `docs/preprocessor.php`, `docs/ac.php`, `docs/as.php`, `docs/cli.php`, `docs/fileheader.php`).

### Dedicated CLI Documentation
- Created dedicated CLI documentation file `DOCS/CLI.md` detailing every AstraC command (`asm`, `comp`, `disasm`, `objdump`, `strdump`, `preproc`, `info`, `showline`, `version`, `help`), sub-arguments (`all`, `header`, `tables`, `funcs`, `relocs`, `imports`), flags (`type {exe|lib} [ot|ft|it]`, `arch`, `bits`, `org`, `entry`, `warn`, `stepoff`, `macro`, `verbose`, `debug`), return codes, and examples.
- Created external website CLI documentation page `C:\xampp\htdocs\astrac\docs\cli.php` matching project website styling.
- Updated all repository markdown files (`README.md`, `AGENTS.md`, `DOCS/AC_FILEHEADER.md`, `DOCS/AC_LANG.md`) and website pages (`index.php`, `docs/fileheader.php`, `docs/ac.php`).

### ACFH Binary Header & Function Export Tables
- Expanded `type {exe|lib} [offset_table(ot)|function_table(ft)|import_table(it)]` CLI argument parsing to support multiple tables combined.
- Expanded `AC_FILE_HEADER` with `func_table_offset`, `func_table_size`, `import_table_offset`, `import_table_size` fields, `AC_FLAG_HAS_FUNCS`, `AC_FLAG_HAS_IMPORTS` flags, preserving 108-byte total header size (`reserved[40]`).
- Implemented Function Export Table generation (`AC_FUNC_TABLE_HDR`, `AC_FUNC_ENTRY`, dynamic string table pool) with parameter stack size metadata (`param_size` in bytes) and calling convention flags (cdecl/stdcall/variadic), resolved via compiler symbol table (`FIND_SYM`/`COMP_TYPE_SIZE`).
- Enhanced `objdump <file.BIN> [all|header|tables|funcs|relocs|imports]` with sub-argument filtering for focused binary table inspection.
- Added compiler test suite `11_acfh_tables.ac` and updated `TESTS/RUN_COMPILER_TESTS.ps1`.

### CI & Release Automation
- Converted release workflow (`.github/workflows/release.yml`) to manual `workflow_dispatch` trigger: supports Major, Minor, or Patch bump selection (`patch`, `minor`, `major`) and optional explicit version overrides (e.g. `1.0.0`).
- Updated `SCRIPTS/UPGRADE_VERSION.py` to support explicit target version strings (`python3 SCRIPTS/UPGRADE_VERSION.py 1.0.0`).
- Automatically commits updated version files (`VERSION/VERSION.h`, `VERSION.txt`, `VERSION.nsh`) back to `main`, tags `v<version>`, and publishes official GitHub Release assets.

## 2026-09-25

### Compiler Fixes
- Fixed AST type verification for arrays of pointers (`COMPILER/VERIFY_AST.c`): `CNODE_INDEX` now checks if the indexed base node is an array (`array_size > 0`), preserving the element type's pointer depth (e.g., `e100_rfd_t *`) instead of stripping pointer depth and calculating stride based on target struct size (e.g., 1544 bytes instead of 4 bytes).
- Added compiler test suite `10_array_of_pointers.ac` verifying array of pointers indexing and pointer dereferencing codegen.

## 2026-09-24

### Compiler Fixes
- Fixed compound boolean expression code generator where `&&` and `||` failed to emit `AND EAX, EBX` and `OR EAX, EBX` instructions.
- Fixed pointer parameter reference bug in codegen that emitted `LEA EAX, [EBP+off]` (loading stack slot address) instead of `MOV EAX, [EBP+off]` (loading pointer value) for pointer parameter expressions.
- Fixed stack offset alignment for nested local variable declarations following array declarations.
- Fixed parser handling for multi-dimensional array declarations (`[N][M]`), anonymous struct/union/enum declarations, and disambiguated function calls from type names.
- Fixed struct type size calculation and global variable `.data` emission: `GEN_TYPE_SIZE` and `FIELD_TYPE_SIZE` now recursively resolve typedef symbols (`SYM_TYPEDEF`), and global/local variable codegen reserves the full `count * struct_size` bytes in `.data` and stack frames.
- Fixed `VERIFY_AST.c` type verification: corrected `IS_POINTER` to recognize pointer depth (`ptr_depth > 0`), added `ARE_TYPES_ASSIGNABLE` helper to permit integer/enum conversions, pointer assignments, and `0`/`NULLPTR` initializers without emitting spurious `assignment type mismatch` warnings.
- Fixed struct member access resolution on typedef'd structs in `VERIFY_AST.c`, `GEN.c`, and inline assembly blocks: member offsets, member array sizes, and types now properly resolve through `SYM_TYPEDEF`.
- Fixed array member and aggregate evaluation: referencing a struct array member (e.g. `entry[j].Name`) now yields the member base address rather than dereferencing the first 4 bytes as a scalar.
- Fixed struct assignment: assigning structs (e.g. `*out_entry = entry[j]` or `dest = src`) now generates memory block copies (`REP MOVSD` / unrolled dword/word/byte copy) instead of storing a 4-byte pointer.
- Fixed local pointer array indexing in `GEN_ARR_BASE`: indexing through a pointer variable (e.g. `entry[j]`) now correctly loads the pointer value (`MOV EAX, [EBP-off]`) instead of loading the stack slot address (`LEA EAX, [EBP-off]`).
- Fixed anonymous struct typedef symbol resolution and type size calculation: `typedef struct { ... } NAME;` now correctly assigns the tag name to the struct symbol so `GEN_TYPE_SIZE` computes exact struct dimensions instead of defaulting or misidentifying global variables.
- Implemented function-level symbol scoping and increased symbol capacity: added `func_name` scope tagging to `SYMBOL` and expanded `SYM_MAX_ENTRIES` from 1024 to 8192. Scoped parser (`SYM_ADD`/`SYM_LOOKUP`), verifier (`V_FIND_SYM`), and code generator (`FIND_SYM`) to function boundaries, preventing variable name collisions across functions and resolving erroneous type overwrite bugs (such as `entry[j]` inheriting struct types and strides from other functions).
- Fixed `continue;` statement branching in `for` loops (`COMPILER/GEN.c`): jumps to the loop update/increment expression label (`lbl_step`) rather than the condition test label (`lbl_start`), ensuring loop counters increment correctly and preventing infinite loops. Also updated `do ... while` to jump to condition evaluation (`lbl_cond`).
- Fixed `CNODE_TERNARY` AST verification (`COMPILER/VERIFY_AST.c`): added verification of the ternary `else` branch (`children[2]`), ensuring struct member accesses inside `else` expressions have their byte offsets (`ival`) properly resolved instead of defaulting to offset 0.

### Assembler Fixes
- Fixed instruction prefix parsing in `ASSEMBLER/AST.c`: prefixes (`REP`, `REPE`, `REPZ`, `REPNE`, `REPNZ`, `LOCK`) are now treated as 0-operand instructions, allowing same-line prefix instructions (e.g., `REP MOVSD`, `LOCK CMPXCHG`) to parse and encode correctly instead of failing with missing mnemonic form errors.

### Testing & Test Suite
- Added comprehensive compiler test suite under `TESTS/COMPILER/` containing 9 test files covering all compiler features: types & variables (`01_types_and_vars.ac`), operators & expressions (`02_operators.ac`), control flow (`03_control_flow.ac`), for-loop continue (`04_for_continue.ac`), ternary expressions with struct offsets (`05_ternary.ac`), structs/unions/packing (`06_structs_unions.ac`), functions/recursion/variadics (`07_functions.ac`), inline assembly (`08_inline_asm.ac`), and function symbol scoping (`09_scoping.ac`).
- Added automated test runner scripts: PowerShell (`TESTS/RUN_COMPILER_TESTS.ps1`) and Bash (`TESTS/RUN_COMPILER_TESTS.sh`) with regression checks on assembly output.
- Added Testing & Verification Rule to `AGENTS.md` mandating test execution for all changes.

### CLI & Tools
- Added `strdump <file.BIN>` command (`STRDUMP.c`) to inspect and dump null-terminated string literals from the `.rodata` section of ACFH executable/library binaries.
- Standardized CLI flags across documentation and removed legacy hyphen prefixes (`--` / `-`).

### CI & Release Automation
- Added standalone GitHub Actions test workflow (`.github/workflows/test.yml`) running matrix compiler tests on Windows (MSVC x64) and Linux (GCC Ninja) on push and pull request to `main` and `development`.
- Integrated parallel test execution into pre-release workflow (`.github/workflows/pre-release.yml`): `build-windows`, `build-linux`, `test-windows`, and `test-linux` execute simultaneously after version bump, gating release publishing on all jobs passing.
- Added GitHub Actions pre-release workflow (`.github/workflows/pre-release.yml`) triggering on pushes to `main` to automatically extract commit changelogs, increment the patch version (`SCRIPTS/UPGRADE_VERSION.py 0 0 1`), commit and tag `v<version>`, and publish a pre-release named `patch-build-<version>` with both Windows NSIS installer and Linux package artifacts attached.
- Fixed Linux package build: corrected source filename casing in `CMakeLists.txt` (`"AstraC.c"` / `"AstraC.h"`) to support case-sensitive Linux filesystems and made `ASTRAC.rc` Windows-only (`if (WIN32)`).
- Fixed Windows NSIS installer job in CI: added automatic NSIS installation (`choco install nsis`) in `.github/workflows/pre-release.yml` and added PATH and Chocolatey discovery to `SCRIPTS/WIN/CREATE_NSIS.BAT`.
- Pinned Windows CI jobs (`test-windows` and `build-windows`) to `windows-2022` to guarantee availability of Visual Studio 17 2022 and added automatic fallback to `cmake -A x64` in workflow files, `SCRIPTS/WIN/CREATE_NSIS.BAT`, and `SCRIPTS/WIN/MAKE.BAT`.
- Added non-interactive `-batch` and `-ci` flag support to `SCRIPTS/WIN/CREATE_NSIS.BAT` and non-interactive handling to `SCRIPTS/SH/MAKE.sh`.

### Documentation & Website
- Updated all AstraC documentation files (`README.md`, `AGENTS.md`, `DOCS/*.md`) and external website files (`C:\xampp\htdocs\astrac`).
- Documented default 1-byte struct packing behavior in `DOCS/AC_LANG.md` and website documentation.
- Added documentation maintenance guidelines to `AGENTS.md`.

