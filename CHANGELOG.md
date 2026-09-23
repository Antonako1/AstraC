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

### CLI & Tools
- Added `strdump <file.BIN>` command (`STRDUMP.c`) to inspect and dump null-terminated string literals from the `.rodata` section of ACFH executable/library binaries.
- Standardized CLI flags across documentation and removed legacy hyphen prefixes (`--` / `-`).

### Documentation & Website
- Updated all AstraC documentation files (`README.md`, `AGENTS.md`, `DOCS/*.md`) and external website files (`C:\xampp\htdocs\astrac`).
- Added documentation maintenance guidelines to `AGENTS.md`.
