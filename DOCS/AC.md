# AstraC Compiler & Language Reference

AstraC is a lightweight C-like systems programming language and compiler for
i386 and i286 x86 architectures. It produces standalone raw binary files with
no object-linking step. The language is case-insensitive and designed for
low-level work -- operating systems, bootloaders, embedded firmware, and
bare-metal applications.

---

## Compiler CLI

```
AstraC.exe comp <file.AC> [flags]

Flags:
    --arch i386|i286   ; Target architecture (default: i386)
    --bits 16|32       ; Code mode (default: 32)
    --entry <label>    ; Override entry point (default: main)
    -E                 ; Stop after preprocessing (emit .AC)
    -S                 ; Stop after codegen (emit .AS)
    --stepoff <1|2|3>  ; Stop after given stage
    --verbose          ; Print progress information
    --warn 0|1|2|err   ; Warning level (err = treat as errors)
    --org <addr>       ; Origin address (hex, no 0x prefix)
    macro <name> <v>   ; Define preprocessor macro
```

### Pipeline stages

| Stage | Description | Stop flag |
|-------|-------------|-----------|
| 1. Preprocessing | Handle `#include`, `#define`, `#if`/`#else`/`#endif` | `-E` / `stepoff 1` |
| 2. Lexical analysis | Tokenize into keywords, identifiers, literals, operators | -- |
| 3. Parsing | Build Abstract Syntax Tree (AST) from token stream | -- |
| 4. Verification | Semantic checks -- types, scopes, forward declarations | -- |
| 5. Code generation | Emit `.AS` assembly source | `-S` / `stepoff 2` |
| 6. Assembly | Assemble `.AS` -> flat `.BIN` binary | `stepoff 3` |

### Output files

```
input.AC -> [Preprocessor] -> /tmp/00.AC -> [Compiler] -> input.AS -> [Assembler] -> input.BIN
```

- With `-E`: outputs preprocessed source to `/tmp/00.AC`
- With `-S`: outputs assembly source to `input.AS`
- Default: outputs binary to `input.BIN`

### Supported architectures

| Flag | Target |
|------|--------|
| `--arch i386` (default) | 32-bit protected mode, full i386 ISA |
| `--arch i286` | 16-bit real mode, 80286 subset |

The `--bits` flag controls the default operand/address size. Use `--bits 16` for
16-bit real-mode code.

### Error codes

| Code | Name | Meaning |
|------|------|---------|
| 0 | `ASTRAC_OK` | Success |
| 1 | `ASTRAC_ERR_ARGS` | Bad or missing arguments |
| 2 | `ASTRAC_ERR_PREPROCESS` | Preprocessor failure |
| 3 | `ASTRAC_ERR_LEX` | Lexer failure |
| 4 | `ASTRAC_ERR_AST` | AST construction failure |
| 5 | `ASTRAC_ERR_VERIFY` | Verification failure |
| 6 | `ASTRAC_ERR_OPTIMIZE` | Optimisation failure |
| 7 | `ASTRAC_ERR_CODEGEN` | Code generation failure |
| 8 | `ASTRAC_ERR_DISASSEMBLE` | Disassembly failure |
| 9 | `ASTRAC_ERR_COMPILE` | Compilation failure |

---

## Lexical Structure

### Comments

Three comment styles are supported:

```c
;  Assembly-style line comment  (useful in asm blocks)
// C-style single-line comment
/* C-style multi-line comment */
```

### Identifiers and keywords

The language is **case-insensitive** -- all identifiers and keywords are
normalized to UPPER CASE during lexing. `MyVar`, `myvar`, and `MYVAR` are the
same identifier.

#### Reserved keywords

| Category | Keywords |
|----------|----------|
| Types | `U8`, `U16`, `U32`, `I8`, `I16`, `I32`, `F32`, `U0`, `VOID`, `BOOL`, `VOIDPTR` |
| Pointers | `PU8`, `PU16`, `PU32`, `PPU8`, `PPU16`, `PPU32`, `PI8`, `PI16`, `PI32`, `PPI8`, `PPI16`, `PPI32` |
| Literals | `TRUE`, `FALSE`, `NULLPTR` |
| Control flow | `IF`, `ELSE`, `FOR`, `WHILE`, `DO`, `RETURN`, `BREAK`, `CONTINUE`, `SWITCH`, `CASE`, `DEFAULT`, `GOTO` |
| Declarations | `STRUCT`, `UNION`, `ENUM`, `TYPEDEF`, `SIZEOF`, `STATIC`, `LOCAL` |
| Misc | `ASM` |

### Numeric literals

| Format | Prefix | Example | Value |
|--------|--------|---------|-------|
| Decimal | _(none)_ | `42`, `1234` | 42, 1234 |
| Hexadecimal | `0x` / `0X` | `0xFF`, `0x100` | 255, 256 |
| Binary | `0b` / `0B` | `0b1010`, `0B11110000` | 10, 240 |
| Character | `'...'` | `'A'`, `'\n'` | 65 (U8), 10 (U8) |

### String and character literals

```c
"Hello, world!"      ; string literal (type PU8, stored in .rodata)
'A'                   ; character literal (type U8)
'\n'                  ; escape sequences supported
```

Escape sequences: `\n` (0x0A), `\r` (0x0D), `\t` (0x09), `\0` (0x00), `\\` (0x5C), `\"` (0x22)

---

## Type System

### Built-in types

| Type | Size (bytes) | Signed | Description |
|------|-------------|--------|-------------|
| `U8` | 1 | No | Unsigned 8-bit integer |
| `U16` | 2 | No | Unsigned 16-bit integer |
| `U32` | 4 | No | Unsigned 32-bit integer |
| `I8` | 1 | Yes | Signed 8-bit integer |
| `I16` | 2 | Yes | Signed 16-bit integer |
| `I32` | 4 | Yes | Signed 32-bit integer |
| `F32` | 4 | IEEE 754 | 32-bit floating point |
| `BOOL` | 4 | No | Boolean -- `TRUE`(1) / `FALSE`(0); first-class type |
| `U0` / `VOID` | 0 | -- | Void/empty type, for no-return functions |
| `VOIDPTR` | 4 | -- | Generic pointer to void |

`BOOL` is a first-class type distinct from `U32`, though both are 4 bytes wide.
`BOOL8` is available as `typedef U8 BOOL8` in user code.
No `U64` -- the compiler targets 32-bit architectures only.

### Pointer types

All pointers are **32-bit** values. Distinct pointer types provide compile-time
type checking:

| Category | Unsigned | Signed |
|----------|----------|--------|
| Single pointers | `PU8`, `PU16`, `PU32` | `PI8`, `PI16`, `PI32` |
| Double pointers | `PPU8`, `PPU16`, `PPU32` | `PPI8`, `PPI16`, `PPI32` |

`PU8` and `PI8` are distinct types despite pointing to same-width data.
No `CONST` qualifier exists.

### sizeof

`sizeof(type)` and `sizeof expr` are evaluated at compile time. Pointers always
return 4.

```c
sizeof(U32)     // 4
sizeof(PU8)     // 4  (all pointers are 32-bit)
```

---

## Implicit Conversions

| From -> To | Behavior |
|-----------|----------|
| U8 -> U16, U32; I8 -> I16, I32 | Allowed (widening, safe) |
| U32 -> U8; I32 -> I8 | Warning (narrowing, possible data loss) |
| I32 -> U32; U32 -> I32 | Allowed with warning (signedness change) |
| U32 -> PU8; PU8 -> U32 | Allowed; warning if signedness incompatible |
| BOOL -> U32; U32 -> BOOL | Allowed |
| VOIDPTR -> other pointer | Allowed with warning |

## Explicit Casting

C-style cast syntax: `(type)expression`. No runtime validation -- developer is
responsible for correctness.

```c
U32 x = (U32)ptr;           // pointer -> integer
PU8  p = (PU8)0xB8000;      // integer -> pointer
U8   b = (U8)x;             // narrowing cast
```

---

## Declarations and Scope

### Storage classes

| Keyword | Meaning |
|---------|---------|
| _(none)_ | Function-local variable (stack) or block-local |
| `static` | Cross-file global -- visible across all files |
| `local` | File-local -- visible only within the defining file |

```c
static U32 global_counter;   // accessible in all files
local  PU8 msg = "Hello";    // accessible only in this file

U32 main() {
    U32 local_var;           // function-local
    if (local_var > 0) {
        U32 block_var;       // block-scoped (C99-style)
    }
}
```

### Scope rules

- **Block scope**: Variables declared inside `{ }` are scoped to that block.
- **Loop variables**: `for (U32 i = 0; ...)` -- `i` is scoped to the loop body.
- **Function scope**: Labels (`goto label:`) are scoped to the defining function.
- **Forward declarations**: Functions must have a prototype before first use.

### typedef

```c
typedef U32   COUNTER;
typedef PU8   STRING;
typedef struct _POINT { U32 x; U32 y; } POINT, *PPOINT;
```

---

## Variables and Storage

### Global variables

Placed in `.data` (mutable) or `.rodata` (read-only). Default to **zero** if no
initializer is provided. There is no `.bss` section.

```c
static U32 counter = 0;         // .data, explicit zero
local  PU8 msg     = "Hello";   // .data -- pointer to .rodata string
```

### Stack variables

Function-local and block-local variables live on the stack. **Aggregate
initialization is not supported** -- initialize field-by-field:

```c
U32 arr[256];
for (U32 i = 0; i < 256; i++) {
    arr[i] = i;
}
```

### Stack arrays

Fixed-size arrays declared with bracket syntax:
```c
U32 buf[256];               // stack array of 256 U32s
PU8 names[10];              // array of 10 pointers
```

Array indexing uses `arr[n]` syntax. Pointers also support `ptr[n]` notation
(equivalent to `*(ptr + n)`).

`MEMZERO` and `MEMSET` are external functions -- the compiler does **not** provide
them. The developer must implement or link these.

---

## Structs, Unions, and Enums

### Structs

```c
typedef struct _STUDENT {
    U32 id;
    PU8 name;
} STUDENT, *PSTUDENT;

U32 main() {
    STUDENT s;
    s.id   = 12345;          // field-by-field init only
    s.name = "John";
    PPOINT p = &s;
    p->id = 0;              // auto-dereference
    return s.id;
}
```

- **Typedef convention**: `typedef struct _NAME { ... } NAME, *PNAME`
- **Anonymous structs/unions**: Supported inside other structs
- **Member access**: `.` for values, `->` for pointers (auto-dereference)
- **Bitfields**: Not supported
- **Flexible array members**: Not supported
- **Aggregate init** (`= { ... }`): Not supported

### Unions

```c
typedef union _VALUE {
    U32 u;
    I32 i;
    F32 f;
} VALUE;
```

### Enums

Underlying type is always `U32`. Enum values can be explicitly assigned:

```c
typedef enum _STATUS {
    OK,              // 0
    WARN,            // 1
    ERR = 5,         // 5
    FATAL            // 6
} STATUS;
```

---

## Operators

Full C-compatible precedence, shown highest to lowest:

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| 1 | `()` `[]` `.` `->` `++` (post) `--` (post) | L -> R |
| 2 | `++` (pre) `--` (pre) `+` (unary) `-` (unary) `!` `~` `*` (deref) `&` (addr) `sizeof` | R -> L |
| 3 | `(type)` cast | R -> L |
| 4 | `*` `/` `%` | L -> R |
| 5 | `+` `-` | L -> R |
| 6 | `<<` `>>` | L -> R |
| 7 | `<` `<=` `>` `>=` | L -> R |
| 8 | `==` `!=` | L -> R |
| 9 | `&` (bitwise AND) | L -> R |
| 10 | `^` (bitwise XOR) | L -> R |
| 11 | `|` (bitwise OR) | L -> R |
| 12 | `&&` (logical AND) | L -> R |
| 13 | `||` (logical OR) | L -> R |
| 14 | `?:` (ternary) | R -> L |
| 15 | `=` `+=` `-=` `*=` `/=` `%=` `<<=` `>>=` `&=` `^=` `|=` | R -> L |

### Key behaviors

- **Assignment is an expression**: `x = y = z = 0` chains right to left.
- **Pre/post increment**: `++x` returns new value, `x++` returns old value.
- **`->` auto-dereference**: `ptr->field` is equivalent to `(*ptr).field`.
- **Relational and logical** expressions produce `BOOL` type.
- **Ternary**: `cond ? t : f` -- true expression if cond is non-zero.

---

## Control Flow

### if / else

```c
if (cond) {
    // ...
} else if (cond2) {
    // ...
} else {
    // ...
}
```

### for loop

Full C-style: `for (init; cond; step)`. Loop counter is scoped to body only.

```c
for (U32 i = 0; i < 10; i++) {
    // i is only visible here
}
```

### while / do-while

```c
while (cond) { /* ... */ }
do { /* ... */ } while (cond);
```

### switch / case

Integer types only. **Fallthrough by default** (like C). `case` values must be
compile-time constants. Use `break` to exit.

```c
switch (val) {
    case 0:
        break;
    case 1:
    case 2:          // fallthrough
        break;
    default:
        break;
}
```

### goto and labels

Labels use plain `label:` syntax, scoped to the defining function. `goto` can
jump to any label within the same function.

### break / continue

`break` exits the innermost loop or switch. `continue` jumps to the next
iteration of the innermost loop.

### return

```c
return;            // void function exit
return expr;       // return value from function
```

---

## Functions

### Declaration and calling convention

```c
RETURN_TYPE name(PARAM_TYPE p1, PARAM_TYPE p2, ...);
```

**cdecl ABI**: Arguments pushed onto the stack right-to-left. Return value in
`EAX` (or `ST0` for `F32`). Caller cleans the stack. Every function gets a
mandatory `PUSH EBP; MOV EBP, ESP` stack frame.

### Entry point

Default entry point: `U32 main(U32 argc, PPU8 argv)`.
Override with `--entry <label>` flag.

`argc` and `argv` are set up by the program loader -- the compiler does not
manage them. If no loader is present, `argc` will be 0 and `argv` will be
`NULLPTR`.

### Void functions

Either `U0 foo()` or `VOID foo()` syntax. Use `return;` with no value.

### Variadic functions

Supported via `...` syntax with `va_list` mechanism:

```c
U32 sum(U32 count, ...) {
    va_list args;
    va_start(args, count);
    U32 total = 0;
    for (U32 i = 0; i < count; i++) {
        total += va_arg(args, U32);
    }
    va_end(args);
    return total;
}
```

### Function pointers

C-style syntax:

```c
typedef U32 (*BINOP)(U32, U32);

U32 apply(BINOP op, U32 a, U32 b) {
    return op(a, b);                   // call through pointer
}

U32 main() {
    BINOP fp = &add;
    return fp(3, 4);                   // returns 7
}
```

---

## Assembly Block

Inline assembly using `asm { ... }`:

```c
U32 main() {
    U32 result;
    asm {
        mov eax, 5
        add eax, 10
        mov result, eax
    }
    return result;
}
```

- The `asm` block is **literally pasted** into the `.AS` output. No validation.
- Variables declared in the enclosing scope are automatically translated to
  their stack addresses (`[EBP+offset]` or `[EBP-offset]`).
- Struct member access (`student.id`) is also translated to the correct EBP offset.
- The asm block is a **clobber-everything barrier**: all registers are assumed
  modified.
- Any valid assembler syntax including labels and raw opcodes is permitted.

---

## Preprocessor

AC shares the preprocessor with the assembler. It provides the full C
preprocessor:

| Directive | Description |
|-----------|-------------|
| `#include "file"` | Include another file (concatenation -- no separate linking) |
| `#define NAME value` | Define object-like macro (no function-like macros) |
| `#undef NAME` | Remove a macro definition |
| `#ifdef NAME` / `#ifndef NAME` | Conditional on macro defined/undefined |
| `#if expr` / `#elif expr` / `#else` | Conditional on preprocessor expression |
| `#endif` | End conditional block |
| `#error "msg"` | Emit error and halt |
| `#warning "msg"` | Emit warning |

### Predefined macros

| Macro | Value | Condition |
|-------|-------|-----------|
| `ARCH_I386` | `1` | Defined when targeting i386 |
| `ARCH_I286` | `1` | Defined when targeting i286 |

### Multi-file compilation

Multiple `.AC` files are combined via `#include`. The preprocessor concatenates
all included files into a single preprocessed source (`/tmp/00.AC`). There is
no separate object-linking step -- everything compiles to one `.BIN`.

---

## Code Generation

### Target

The compiler emits x86 `.AS` assembly source, then assembles to flat binary.
Default mode is 32-bit protected mode (i386). 16-bit real mode via `--bits 16`.

### Memory layout

| Section | Content |
|---------|---------|
| `.data` | Mutable globals -- initialized or default-zero |
| `.rodata` | Read-only data -- string literals |
| `.code` | All generated instructions |

No `.bss` section. Uninitialized globals default to zero in `.data`.

### Calling convention details

- Arguments pushed right-to-left
- `CALL` instruction pushes return address
- `PUSH EBP; MOV EBP, ESP` prologue on every function
- Local variables at `[EBP-N]`, parameters at `[EBP+8+4*N]`
- Return value in `EAX`
- Caller cleans stack with `ADD ESP, N`
- All 32-bit pointers, 4-byte stack alignment

### External dependencies

The compiler does **not** provide any runtime library. Functions like `MEMZERO`,
`MEMSET`, `memcpy` must be implemented by the developer or linked from an
external runtime.

---

## Complete Example

```c
#include "stdlib.ah"

typedef struct _STUDENT {
    U32 id;
    PU8 name;
} STUDENT, *PSTUDENT;

PU8 get_student_name(PSTUDENT student) {
    return student->name;
}

U32 main(U32 argc, PPU8 argv) {
    STUDENT student;
    student.id   = 12345;
    student.name = "John Doe";

    if (student.id > 10000) {
        for (U32 i = 0; i < 5; i++) {
            asm {
                mov eax, student.id
                add eax, i
                mov student.id, eax
            }
        }
    }

    switch (student.id) {
        case 0:  return 0;
        case 50: return 50;
        default: break;
    }

    PU8 name = get_student_name(&student);
    return student.id;
}
```