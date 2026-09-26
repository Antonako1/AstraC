# Changelog

All notable changes to AstraC will be documented in this file.

## 2026-09-26

### ACFH Binary Header & Function Export Tables
- Expanded `type {exe|lib} [offset_table(ot)|function_table(ft)]` CLI argument parsing to support multiple tables combined.
- Expanded `AC_FILE_HEADER` with `func_table_offset` and `func_table_size` fields, `AC_FLAG_HAS_FUNCS` flag, preserving 108-byte total header size (`reserved[48]`).
- Implemented Function Export Table generation (`AC_FUNC_TABLE_HDR`, `AC_FUNC_ENTRY`, dynamic string table pool) with parameter stack size metadata (`param_size` in bytes) and calling convention flags (cdecl/stdcall/variadic), resolved via compiler symbol table (`FIND_SYM`/`COMP_TYPE_SIZE`).
- Enhanced `objdump <file.BIN> [all|header|tables|funcs|relocs]` with sub-argument filtering for focused binary table inspection.
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

