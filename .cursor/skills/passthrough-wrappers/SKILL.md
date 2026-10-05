---
name: passthrough-wrappers
description: >-
  Find and remove C++ passthrough wrappers that only forward the same
  parameter count and types to another function. Use when the user asks to
  review wrappers, find rename-only adapters, audit thin forwards, remove
  unnecessary wrap_* helpers, or names a specific function to check as a
  wrapper.
---

# Passthrough wrapper review

Normative rule: [`AGENTS.md`](../../../AGENTS.md) under **Wrappers (no passthrough renames)**. Do not invent parallel rules.

## Scope modes

1. **Named function** — user names a symbol. Check whether it is a passthrough wrapper of some callee; analyze its body and call sites only unless they ask to expand.
2. **File / module review** — scan free functions (and obvious methods) in the named `.cpp`/`.hpp` for passthrough bodies.
3. **Broader refactor** — user asks to remove wrappers across the tree; start from named files, then search for thin `return other(...)` forwards.

Default to the narrowest mode. Prefer report-then-edit unless the user asked to refactor/apply.

## Detection checklist

For each candidate:

- [ ] Does the body essentially only call another function and return its result (or void-call it)?
- [ ] Are arguments forwarded with the **same count and types** as the candidate’s parameters (possibly identical names)?
- [ ] Is there **no** preprocessing: no parsing, slicing, validation that changes values, derivation of new arguments, aggregation, or other semantic work before the call?
- [ ] Verdict: **passthrough (remove)** or **legitimate wrapper (keep)**

### Verdicts

| Verdict | Action |
|---------|--------|
| Passthrough | Delete the wrapper; update call sites to call the target (or rename the target if the wrapper’s name was the desired API) |
| Legitimate | Keep — parameters are processed or derived before the target call |
| Ambiguous | Report and ask; e.g. only `const&` vs by-value with no other change, or logging-only side effects |

Reordering or renaming parameters without changing meaning still counts as passthrough. A different outer signature that **computes** the inner arguments is legitimate.

## Workflow

1. Read **Wrappers** in `AGENTS.md`.
2. Resolve scope (named function vs file vs broader).
3. Find candidates: functions whose body is dominated by a single call to another symbol.
4. Apply the checklist; record callee and whether args are unchanged forwards.
5. **If review only:** report (see output format). Do not edit.
6. **If refactor requested:** remove passthroughs; point callers at the real function; keep `AGENTS.md` style (including variable declarations at scope start + blank line after decls); compile if a build exists. When touching call sites or leftover helpers, prefer [variable-declarations](../variable-declarations/SKILL.md) if declaration placement is wrong.
7. Summarize removals and anything deferred.

## Output format

```markdown
## Scope
[named function | files | broader]

## Findings
| Symbol | Callee | Same arity/types forwarded? | Processing before call? | Verdict | Action |
|--------|--------|------------------------------|-------------------------|---------|--------|
| ... | ... | yes/no | yes/no | passthrough / legitimate | remove / keep / ask |

## Changes
- [what was edited, or "review only — no edits"]

## Deferred
- [ambiguous cases needing user confirmation]
```

## Named-function focus

When the user asks about one function:

1. Open its definition; identify the callee (if any).
2. Compare parameter lists and the arguments in the call.
3. Say clearly whether it is a forbidden passthrough or an accepted processing wrapper, with a short cite of the body.
4. If removing, list call sites that must switch to the callee (or propose renaming the callee instead).
