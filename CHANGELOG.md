# Changelog

All notable changes to AstraC will be documented in this file.

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

### Assembler Fixes
- Fixed instruction prefix parsing in `ASSEMBLER/AST.c`: prefixes (`REP`, `REPE`, `REPZ`, `REPNE`, `REPNZ`, `LOCK`) are now treated as 0-operand instructions, allowing same-line prefix instructions (e.g., `REP MOVSD`, `LOCK CMPXCHG`) to parse and encode correctly instead of failing with missing mnemonic form errors.

### CLI & Tools
- Added `strdump <file.BIN>` command (`STRDUMP.c`) to inspect and dump null-terminated string literals from the `.rodata` section of ACFH executable/library binaries.
- Standardized CLI flags across documentation and removed legacy hyphen prefixes (`--` / `-`).

### CI & Release Automation
- Added GitHub Actions pre-release workflow (`.github/workflows/pre-release.yml`) triggering on pushes to `main` to automatically extract commit changelogs, increment the patch version (`SCRIPTS/UPGRADE_VERSION.py 0 0 1`), commit and tag `v<version>`, and publish a pre-release named `patch-build-<version>` with both Windows NSIS installer and Linux package artifacts attached.
- Added non-interactive `-batch` and `-ci` flag support to `SCRIPTS/WIN/CREATE_NSIS.BAT`.

### Documentation & Website
- Updated all AstraC documentation files (`README.md`, `AGENTS.md`, `DOCS/*.md`) and external website files (`C:\xampp\htdocs\astrac`).
- Added documentation maintenance guidelines to `AGENTS.md`.

