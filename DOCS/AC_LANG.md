# AC Language Specification

AC is a C-like systems programming language for i386 and i286 x86 architectures.
It produces standalone raw binaries with no object-linking step. The language is
case-insensitive and designed for low-level work -- operating systems,
bootloaders, embedded firmware, and bare-metal applications.

## Compilation pipeline

```
.AC source -> Preprocessor -> Lexer -> Parser -> Verifier -> Codegen -> Assembler -> .BIN
```

| Stage | Description | Stop flag |
|-------|-------------|-----------|
| Preprocess | Handle `#include`, `#define`, `#if`/`#else`/`#endif` | `-E` / `stepoff 1` |
| Lex | Tokenize into keyword, identifier, literal, and operator tokens | -- |
| Parse | Build AST from token stream | -- |
| Verify | Semantic checks -- types, scopes, forward declarations | -- |
| Codegen | Emit `.AS` assembly source | `-S` / `stepoff 2` |
| Assemble | Assemble `.AS` -> flat `.BIN` binary | `stepoff 3` |

The pipeline is controlled via CLI flags:

```
AstraC.exe comp <file.AC> [flags]

    --arch i386|i286    ; Target architecture (default: i386)
    --bits 16|32        ; Code mode (default: 32)
    --entry <label>     ; Override entry point (default: main)
    -E                  ; Stop after preprocessing
    -S                  ; Stop after codegen (emit .AS)
    --stepoff <1|2|3>   ; Stop after given stage
    --verbose           ; Print progress
```

---

## Lexical structure

### Comments

AC supports three comment styles:

```c
;  Assembly-style line comment  (anywhere -- stripped by preprocessor)
// C-style single-line comment
/* C-style multi-line comment */
```

`;` comments are available throughout (inherited from the assembler). `//` and
`/* */` are available in all contexts.

### Identifiers and keywords

Language is **case-insensitive**. All identifiers and keywords are normalized to
UPPER CASE during lexing. `MyVar`, `myvar`, and `MYVAR` are the same identifier.

Keywords are reserved and cannot be used as identifiers:

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

Unsuffixed integer literals are typed as the smallest unsigned integer type that
fits the value. Negative constants use the `-` unary operator.

### String literals

Double-quoted strings: `"Hello, world!"`. Escape sequences:

| Escape | Value | Description |
|--------|-------|-------------|
| `\n` | 0x0A | Newline |
| `\r` | 0x0D | Carriage return |
| `\t` | 0x09 | Horizontal tab |
| `\0` | 0x00 | Null terminator |
| `\\` | 0x5C | Literal backslash |
| `\"` | 0x22 | Literal double quote |

String literals have type `PU8`, are stored in the `.rodata` section, and are
**not** const-enforced -- modifying them is undefined behavior.

### Character literals

Single-quoted characters: `'A'`, `'\n'`. Type is `U8`.

---

## Type system

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
| `BOOL` | 4 | No | Boolean -- `TRUE`(1) / `FALSE`(0); first-class type, distinct from U32 |
| `U0` / `VOID` | 0 | -- | Void/empty type, used for no-return functions |
| `VOIDPTR` | 4 | -- | Generic pointer to void |

The standard `BOOL` type is the same width as `U32` at the ABI level but tracked
as a distinct type by the compiler. `BOOL8` is not a built-in keyword -- it is
available as a `typedef U8 BOOL8` in user code.

### Pointer types

All pointers are **32-bit** values. Distinct pointer types provide compile-time
type checking:

| Category | Unsigned | Signed |
|----------|----------|--------|
| Single pointers | `PU8`, `PU16`, `PU32` | `PI8`, `PI16`, `PI32` |
| Double pointers | `PPU8`, `PPU16`, `PPU32` | `PPI8`, `PPI16`, `PPI32` |

`PU8` and `PI8` are distinct types despite pointing to the same-width data.
Pointers are writable -- AC has no `CONST` qualifier.

`VOIDPTR` is a distinct generic pointer type. It is **not** the same as `PU8`.

### `sizeof`

`sizeof(type)` and `sizeof expr` are evaluated at compile time.

```c
sizeof(U32)     // 4
sizeof(PU8)     // 4  (all pointers are 32-bit)
sizeof(F32)     // 4
```

---

## Implicit conversions

| From -> To | Behavior |
|-----------|----------|
| U8 -> U16, U32; I8 -> I16, I32 | Allowed (widening, safe) |
| U32 -> U8; I32 -> I8 | **Warning** (narrowing, possible data loss) |
| I32 -> U32; U32 -> I32 | Allowed with **warning** (signedness change) |
| U32 -> PU8; PU8 -> U32 | Allowed; **warning** if signedness incompatible (e.g. I32 -> PPI32 warns) |
| BOOL -> U32; U32 -> BOOL | Allowed |
| BOOL -> PU8 | **Warning** |
| VOIDPTR -> PU8; VOIDPTR -> PI32 | Allowed with **warning** (type incompatibility) |

Warnings can be supressed with explicit casts.

---

## Explicit casting

C-style cast syntax: `(type)expression`. The compiler performs no runtime
validation; the developer is responsible for correctness.

```c
U32 x = (U32)ptr;          // pointer -> integer
PU8  p = (PU8)0xB8000;     // integer -> pointer
U8   b = (U8)x;            // narrowing -- no runtime check
```

---

## Declarations and scope

### Storage classes

| Keyword | Meaning |
|---------|---------|
| _(none)_ | Function-local variable (stack) or block-local |
| `static` | **Cross-file** global -- visible across all files (opposite of C convention) |
| `local` | **File-local** -- visible only within the defining file |

```c
static U32 global_counter;   // accessible in all files
local  U32 file_counter;     // accessible only in this file

U32 main() {
    U32 local_var;           // function-local
    if (local_var > 0) {
        U32 block_var;       // block-scoped (only this block)
    }
    // block_var not visible here
}
```

### Scope rules

- **Block scope**: Variables declared inside `{ }` are scoped to that block (C99-style).
- **Loop variables**: `for (U32 i = 0; ...)` -- `i` is scoped to the loop body only.
- **Function scope**: Labels (`goto label:`) are scoped to the defining function.
- **Forward declarations**: Functions must have a prototype before first use:

```c
U32 add(U32 a, U32 b);      // forward declaration
U32 main() { return add(1, 2); }
U32 add(U32 a, U32 b) { return a + b; }  // definition
```

### `typedef`

Type aliases for all types, including structs, unions, and enums:

```c
typedef U32   COUNTER;
typedef PU8   STRING;
typedef struct _POINT { U32 x; U32 y; } POINT, *PPOINT;
```

---

## Variables and storage

### Global variables

Globals are placed in `.data` (mutable) or `.rodata` (read-only). If no
initializer is provided, the value defaults to **zero**. There is no `.bss`
section.

```c
U32  counter = 0;           // .data, explicit zero
PU8  msg     = "Hello";     // .data -- pointer to .rodata string
```

### Stack variables

Function-local and block-local variables live on the stack. **Aggregate
initialization is not supported** -- initialize field-by-field:

```c
U32 arr[256];
MEMZERO(arr, 256 * sizeof(U32));     // zero-fill (user-provided function)

for (U32 i = 0; i < 256; i++) {
    arr[i] = i;                      // field-by-field
}
```

### Stack arrays

Fixed-size arrays are declared with bracket syntax:

```c
U32 buf[256];               // stack array of 256 U32s
PU8 names[10];              // array of 10 pointers
```

Array indexing uses `arr[n]` syntax. Pointer types also support `ptr[n]`
notation (equivalent to `*(ptr + n)`).

`MEMZERO` and `MEMSET` are external functions -- the compiler does **not**
provide them. The developer must implement or link these.

---

## Structs, unions, and enums

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
    return s.id;
}
```

- **Typedef convention**: `typedef struct _NAME { ... } NAME, *PNAME`
- **Anonymous structs/unions**: Supported inside other structs
- **Member access**: `.` for values, `->` for pointers (auto-dereference)
- **Bitfields**: Not supported
- **Flexible array members**: Not supported
- **Aggregate init** (`= { ... }`): Not supported -- assign fields individually

### Unions

```c
typedef union _VALUE {
    U32 u;
    I32 i;
    F32 f;
} VALUE;
```

### Enums

Enum underlying type is always `U32`:

```c
typedef enum _COLOR {
    RED,        // 0
    GREEN,      // 1
    BLUE = 5,   // 5
    YELLOW      // 6
} COLOR;
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

- **Assignment is an expression**: `x = y = z = 0` -- chains right to left.
- **Post-increment**: `x = y++` assigns old value of `y` to `x`, then increments `y`.
- **Pre-increment**: `x = ++y` increments `y`, then assigns to `x`.
- **`->` auto-dereference**: `ptr->field` is equivalent to `(*ptr).field`.
- **Relational and logical** expressions produce `BOOL` type.

---

## Control flow

### `if` / `else`

```c
if (cond) {
    // ...
} else if (cond2) {
    // ...
} else {
    // ...
}
```

### `for` loop

Full C-style: `for (init; cond; step)`. Loop counter scoped to body only.

```c
for (U32 i = 0; i < 10; i++) {
    // i is only visible here
}
// i not visible here
```

### `while` / `do`-`while`

```c
while (cond) {
    // ...
}

do {
    // ...
} while (cond);
```

### `switch` / `case`

Integer types only. **Fallthrough by default** (like C). `case` values must be
compile-time constants. Use `break` to exit.

```c
switch (val) {
    case 0:
        // ...
        break;
    case 1:
        // fallthrough to case 2
    case 2:
        // ...
        break;
    default:
        // ...
        break;
}
```

### `goto` and labels

Labels use plain `label:` syntax and are scoped to the defining function.
`goto` can jump to any label within the same function.

```c
U32 main() {
    U32 x = 0;
loop:
    x++;
    if (x < 10) goto loop;
    return x;
}
```

### `break` / `continue`

`break` exits the innermost loop or switch. `continue` jumps to the next
iteration of the innermost loop.

### `return`

```c
return;            // void function exit
return expr;       // return value from function
```

---

## Functions

### Declaration syntax

```c
RETURN_TYPE name(PARAM_TYPE param1, PARAM_TYPE param2, ...);
```

### Calling convention

**cdecl**: Arguments pushed onto the stack right-to-left. Return value in `EAX`
(or `ST0` for `F32`). Caller cleans the stack.

### Stack frame

Every function gets a mandatory stack frame:

```asm
PUSH EBP
MOV  EBP, ESP
; ... function body ...
POP  EBP
RET
```

No leaf-function optimization or `__noframe__` attribute is available.

### Entry point

Default entry point: `U32 main(U32 argc, PPU8 argv)`. Override with `--entry`.

`argc` and `argv` are set up by the program loader or bootloader -- the compiler
does not manage them. If no loader is present, `argc` will be 0 and `argv` will
be `NULLPTR`.

### Void functions

Either `U0 foo()` or `VOID foo()` syntax. Use `return;` with no value.

### Variadic functions

Supported via `...` syntax with a `va_list` mechanism:

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

C-style syntax for declaration and call:

```c
typedef U32 (*BINOP)(U32, U32);

U32 apply(BINOP op, U32 a, U32 b) {
    return op(a, b);
}

U32 add(U32 a, U32 b) { return a + b; }

U32 main() {
    BINOP fp = &add;
    return fp(3, 4);        // returns 7
}
```

---

## Assembly block

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

Rules:
- The `asm` block is **literally pasted** into the `.AS` output. The compiler
  does not parse or validate its contents.
- Variables declared in the enclosing scope are **visible by name** inside the
  asm block on the stack (via `[EBP+offset]`).
- The asm block is a **clobber-everything barrier**: the compiler assumes all
  registers may be modified and preserves nothing across the block.
- It is the developer's responsibility to preserve any needed values.
- Raw numeric opcodes, labels, and any valid assembler syntax are permitted.

---

## Preprocessor

AC shares the preprocessor with the assembler (C mode). It provides the full
C preprocessor functionality:

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

```c
#ifdef ARCH_I386
    U32 page_dir[1024];
#elif ARCH_I286
    U16 segment;
#endif
```

### Multi-file compilation

Multiple `.AC` files are combined via `#include`. The preprocessor concatenates
all included files into a single preprocessed source (`/tmp/00.AC`). There is
no separate object-linking step -- everything compiles to one `.BIN`.

---

## Code generation

### Target

The compiler emits x86 `.AS` assembly source, which the assembler then converts
to a flat binary. Default mode is 32-bit protected mode (i386). 16-bit real mode
is enabled via `--bits 16`.

### Memory layout

| Section | Content |
|---------|---------|
| `.data` | Mutable globals -- initialized or default-zero |
| `.rodata` | Read-only data -- string literals, const globals |
| `.code` | All generated instructions |

There is no `.bss` section. Uninitialized globals default to zero in `.data`.

### Strings

String literals are stored in `.rodata` as `PU8`. The compiler does not enforce
const-correctness -- modifying a string literal is undefined behavior at runtime
(and may fault on hardware-protected `.rodata`).

### External functions

`MEMZERO`, `MEMSET`, `memcpy`, and similar utility functions are **not** provided
by the compiler. Developers must implement their own or include them from a
runtime library. The compiler can optionally inline memory operations via flags.

---

## Complete example

```c
#include "stdlib.ah"

typedef struct _STUDENT {
    U32 id;
    PU8 name;
} STUDENT, *PSTUDENT;

PU8 get_student_name(PSTUDENT student) {
    return student->name;
}

U32 increment_student_id(PSTUDENT student) {
    student->id++;
    return student->id;
}

typedef U32 (*IDCALLBACK)(PSTUDENT);

U32 main(U32 argc, PPU8 argv) {
    STUDENT student;
    student.id   = 12345;
    student.name = "John Doe";

    IDCALLBACK cb = &increment_student_id;
    cb(&student);

    U8 buf[16];

    if (student.id > 12345) {
        for (U32 i = 0; i < 5; i++) {
            asm {
                mov eax, student.id
                add eax, i
                mov student.id, eax
            }
        }
    } else {
        student.id = 0;
    }

    switch (student.id) {
        case 0:  return 0;
        case 50: return 50;
        default: break;
    }

    U32 n = 0;
    while (n < student.id) {
        n++;
        if (n == 10) break;
    }

    PU8 name = get_student_name(&student);

    return student.id;
}
```

This example exercises: typedef struct, member access (`.` and `->`), function
pointers, `for` loop with scoped counter, `if`/`else`, `switch`/`case`
(fallthrough), `while` loop, `break`, stack array, `asm` block, and function
calls with pointer arguments.