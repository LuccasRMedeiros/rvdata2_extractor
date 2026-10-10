# AGENTS.md — rvdata2_extractor

This document is the source of truth for AI coding agents working in this repository.
Companion files for other tools (`CLAUDE.md`, `GEMINI.md`, `.cursor/rules/`, etc.) point here; do not duplicate these rules elsewhere.

## Project purpose and boundaries

This project is an **rvdata2 extractor/compressor**. Its goal is to let developers extract and recompress RPG Maker VX Ace `.rvdata2` (and related) game data so that files can be edited with tools other than the main editor, then packed back for use by the game.

Agents must stay within that boundary:

- Prefer work that advances extract, compress, parse, serialize, or build/tooling support for that workflow.
- Do not expand into unrelated products, UIs, or game engines unless the user explicitly asks.

## Language and platform

- Language: **C++**
- Standard: **C++17**
- Libraries: prefer **POSIX** APIs for portability-sensitive OS work
- Build: must build with **GNU build essentials** and **CMake**
- Target: **cross-platform** compatibility (design and dependencies with that in mind)

## Build-file discipline

Agents must be careful when editing build files (`CMakeLists.txt`, `*.cmake`, toolchain snippets).

Rules:

1. Prefer **not** bloating the root `CMakeLists.txt`.
2. When new build logic is needed, add focused **`.cmake` scripts** and load them from CMake only under the circumstances that require them.
3. Whenever build scripts change, **always update `CHANGELOG.md`** in the same change set.
4. Changelog entries must be clear for human reviewers: state **what** changed and **why**.

## Source and header layout

| Kind    | Extension |
|---------|-----------|
| Source  | `.cpp`    |
| Header  | `.hpp`    |

- Class **definitions** live in headers.
- Class **implementations** live in source files.
- Each header must use include guards via `#define`, and all namespaces, classes, methods, properties, macros, and types that the header exposes must be prototyped between the guards.

### Header order

Inside a header, always use this order:

1. Header guards (`#ifndef` / `#define` … `#endif`)
2. `#include`s
3. Macros
4. Type definitions
5. Free functions
6. Classes and class members
7. Namespaces (only when justified — see below), with their contents in the same relative order

### Namespaces (must have a purpose)

Do **not** invent a namespace merely to hold a free function or a class. Prefer symbols at global scope inside the header/source that owns them (e.g. `extract_scripts(...)`, `class MarshalValue`), not `scripts_extract::extract` or `foo::Foo` for a lone entity.

Use a named namespace only when it has a clear purpose, such as:

- Grouping a **coherent multi-symbol API** (several related functions and/or types that form one module surface)
- Preventing **real name collisions** across distinct module APIs that would otherwise clash

Do **not** use a named namespace when:

- There is only one free function (or a thin 1:1 wrapper around one operation)
- There is only one class with little or no related free API beside it
- The namespace name just restates the file or function name (`scripts_extract`, `zlib_util` as a bag for one helper, etc.)

Anonymous namespaces are a **different** purpose from public API namespacing: they provide internal linkage for file-local symbols. See **Helper vs actual functions** below for when a function is a helper and therefore must use an anonymous namespace.

If a named namespace is justified, do **not** bring that namespace into scope with `using` in the source that implements it. Reserve `using` for foreign namespaces you consume.

### Helper vs actual functions

Treat free functions as either **helpers** or **actual** (reusable) functions. Placement follows from that classification.

**Helper functions** are local to one use site or one translation unit. By context they are not expected to be called elsewhere. Typical signs:

- Used in only one place (or only inside one `.cpp`)
- Tied to a specific surrounding procedure (naming, formatting, one-off validation, a private step of a larger algorithm)
- Parameters or logic are concrete to that call site, not a general abstraction

Helpers **must** live in an anonymous namespace in the `.cpp` that owns them. That includes helper types and constants that are only used there. Do not leave them as bare file-scope symbols, declare them in a header “just in case,” or invent a named `detail` / `internal` namespace unless the user explicitly asks.

```cpp
// in foo.cpp — helper: only supports extract_scripts in this file
namespace {
    std::string format_index(size_t index)
    {
        // ...
    }
}
```

**Actual functions** are the reusable API: operations you intentionally share because reuse is real or clearly warranted. Promote (or introduce) an actual function when:

- Parameters are abstract enough to process inputs agnostically (callers pass data; the function does not assume one exclusive call site)
- The same procedure already repeats in other places, or an internal pattern is common across functions and should be factored once
- Other modules need to call it — then declare it in a header and implement it in a source file (without a pointless wrapping named namespace; see above)

Do **not** extract a helper into a public/header actual function on speculation. Wait until reuse or a repeating pattern justifies it. Conversely, do **not** leave a clearly repeated agnostic pattern copy-pasted across call sites when it should become an actual function.

```cpp
// header: actual/reusable — abstract inputs, intended for shared use
std::vector<uint8_t> read_file(const std::string &path);

// .cpp public API: actual function for this module
void extract_scripts(const std::string &input_path, const std::string &output_dir);
```

For an eventual review or refactor pass (including when the user names a specific function), use the project skill [`.cursor/skills/helper-vs-actual/SKILL.md`](.cursor/skills/helper-vs-actual/SKILL.md). The definitions in this section remain authoritative.

### Wrappers (no passthrough renames)

Do **not** add a function that only forwards the same parameters (same count and types) to another function. That is just another name for the callee and is unnecessary.

Forbidden (passthrough / rename-only), assuming `int foo(std::string bar, int a, int b)`:

```cpp
int wrap_foo(std::string bar, int a, int b)
{
    return foo(bar, a, b);
}
```

Allowed only when the wrapper **processes** inputs (or otherwise derives arguments) before calling the target:

```cpp
int wrap_foo(std::string s)
{
    std::string bar = s.substr(0, 12);
    int a = static_cast<int>(s.size());
    int b = a % 8;

    return foo(bar, a, b);
}
```

A thin adapter that only renames, reorders without semantic change, or re-exports the same signature is not a valid wrapper. Call `foo` directly (or give the real function the name you wanted).

For review or removal of passthrough wrappers (including when the user names a specific function), use [`.cursor/skills/passthrough-wrappers/SKILL.md`](.cursor/skills/passthrough-wrappers/SKILL.md).

### Verifiers (no equality renames)

Do **not** add a function or method whose body only checks whether a value equals (or differs from) an expected value and returns that bool. That is just another name for `==` / `!=` and adds call-stack noise without useful abstraction.

Forbidden (trivial verifier / equality rename), assuming a public discriminant the caller can already read:

```cpp
bool is_array(void) const
{
    return type == Type::Array;
}

bool verify_version(int version)
{
    return version == 4;
}
```

Compare at the call site instead:

```cpp
if (root.type != Type::Array)
{
    // ...
}

if (version != 4)
{
    // ...
}
```

Names such as `is_*`, `verify_*`, `check_*`, or `expect_*` do not make the pattern acceptable when the body is still only equality.

Allowed only when the predicate has **real** logic beyond a single comparison — multiple conditions, range/set checks, parsing, normalization, throwing on invalid state as part of a typed accessor, or other non-trivial work:

```cpp
bool is_path_hostile(unsigned char ch)
{
    if (ch < 0x20)
    {
        return true;
    }
    switch (ch)
    {
        case '/':
        case '\\':
        case ':':
            return true;
        default:
            return false;
    }
}
```
Type-tag or enum façade methods over a public member the caller can compare directly (e.g. `is_nil`, `is_bool`, `is_integer`, `is_string`, `is_symbol`, `is_array`) are forbidden.

For review or removal of trivial verifiers (including when the user names a specific function), use [`.cursor/skills/trivial-verifiers/SKILL.md`](.cursor/skills/trivial-verifiers/SKILL.md).

### `#endif` comments (obligatory)

Whenever you close `#ifdef MACRO`, `#ifndef MACRO`, or `#if (EXPR)` with `#endif`, the `#endif` **must** be followed by a comment naming the condition being closed:

```cpp
#endif // __FOO_HPP__
#endif // DFT_COMP
#if defined(FOO)
// ...
#endif // defined(FOO)
```

### Header example

```cpp
// foo.hpp
#ifndef __FOO_HPP__
#define __FOO_HPP__

#include <cmath>

#define VALUE_A    128

typedef enum {
    dft = 0,
    opt_a,
    opt_b
} foo_e;

class Foo {
    private:
        int a;

    public:
        void set_a(int a);
        int get_a(void);
};

#endif // __FOO_HPP__
```

## Source file conventions

- Implementations are mainly functions and methods.
- If a source file needs file-scope macros or custom types, follow the same order as headers (after includes).
- Include the matching header first, then other headers.
- Prefer free functions and classes without a wrapping named namespace unless the namespace rule above applies.
- Classify new free functions as helpers or actual functions (see above); helpers go in an anonymous namespace, actual functions are the reusable header/source API.
- When implementing inside a justified named namespace, do **not** bring that namespace into scope with `using`. Reserve `using` for namespaces whose types/functions you consume.

### Source example

```cpp
// foo.cpp
#include "foo.hpp"
#include <iostream>

#ifndef DFT_COMP
#define DFT_COMP 64
#endif // DFT_COMP
#define UNDESIRED (DFT_COMP / 2)

using namespace std;

void Foo::set_a(int a)
{
    if (a % DFT_COMP == UNDESIRED)
    {
        this->a = 0;
        cout << "WARNING: Invalid number" << endl;
        return;
    }
    this->a = a;
}

int Foo::get_a(void)
{
    return this->a;
}
```

## Brace and control-flow style

| Construct                         | Opening brace placement                          |
|-----------------------------------|--------------------------------------------------|
| `namespace`, `class`, function    | Same line as the declaration                     |
| `if`, `for`, `while`, `switch`    | Next line                                        |
| `else if`                         | After the previous `}`; its `{` still on next line |
| `do` / `while`                    | `while` follows the closing `}` of the `do` body |

Examples:

```cpp
class Foo {
    public:
        void bar(void);
};

void Foo::bar(void)
{
    if (condition)
    {
        // ...
    }
    else if (other)
    {
        // ...
    }

    do
    {
        // ...
    } while (again);
}
```

## Variable declarations

Declare variables at the **beginning of their scope**. Prefer the start of the function. Declaring at the start of a narrower block (`if` / `else` / loop / nested `{ }`) is acceptable when the variable is only needed there.

After the declaration block in a scope, add a **blank line** before other statements (assignments, calls, control flow).

```cpp
void foo(int val)
{
    int a;
    int b;

    a = val + 1;
    b = 8 + a / 2;
    if (b > 10)
    {
        int c;

        c = (b - 8) * 2 - 1;
    }
}
```

Do **not** interleave declarations with logic mid-scope when the variable belongs to that whole scope (e.g. declare `a` and `b` at the top of `foo`, not just before first use halfway through). Initialization at the declaration site is fine (`int a = val + 1;`) as long as declarations still come first in the scope, followed by a blank line before non-declaration statements when any remain.

For review or refactor of declaration placement (including a named function), use [`.cursor/skills/variable-declarations/SKILL.md`](.cursor/skills/variable-declarations/SKILL.md).

## Globals and resources

- **Avoid globals.** Add them only when the user specifically asks.
- Keep every resource a function works with inside that function’s scope when possible.
- Share resources through **pointers** and **references** when needed.
- For mutexes and similar handles: the code that **acquires** is responsible for **releasing**. Pass them explicitly; do not hide ownership in globals.

## Checklist for agents

Before finishing a change, confirm:

- [ ] Work stays within extractor/compressor scope
- [ ] C++17 + POSIX + CMake/GNU toolchain expectations are respected
- [ ] New build logic prefers `*.cmake` modules; `CHANGELOG.md` updated if build files changed
- [ ] Headers/sources use `.hpp` / `.cpp` and the required section order
- [ ] Named namespaces exist only when they have a clear multi-symbol / collision purpose
- [ ] Helpers vs actual functions are classified correctly; helpers are in anonymous namespaces; actual functions are only promoted when reuse/pattern warrants it
- [ ] No passthrough wrappers (same parameter count/types forwarded unchanged); wrappers must process or derive arguments
- [ ] No trivial verifiers (functions that only return whether a value equals an expected value); compare at the call site
- [ ] Every `#endif` has a condition comment
- [ ] Brace style matches the table above
- [ ] Variables are declared at the start of their scope with a blank line after the declaration block
- [ ] No new globals unless explicitly requested
- [ ] Resource ownership (acquire/release) is clear, especially for mutexes
