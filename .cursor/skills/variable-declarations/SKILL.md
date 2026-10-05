---
name: variable-declarations
description: >-
  Review or refactor C++ variable declaration placement: declare at the start
  of the scope (prefer function; branch/loop only when local-only), with a
  blank line after the declaration block. Use when the user asks to fix
  declaration style, move locals to the top of a scope, audit mid-scope
  declarations, or names a specific function to check.
---

# Variable declaration style review

Normative rule: [`AGENTS.md`](../../../AGENTS.md) under **Variable declarations**. Do not invent parallel rules.

## Scope modes

1. **Named function** — check only that function’s body (and nested scopes) unless the user asks to expand.
2. **File / module review** — scan functions in the named `.cpp`/`.hpp`.
3. **Broader refactor** — start from named files; fix declaration placement where it clearly violates the rule.

Default to the narrowest mode. Prefer report-then-edit unless the user asked to refactor/apply.

## Detection checklist

For each scope (`function` / `if` / `else` / loop / nested block):

- [ ] Are locals used in that scope declared at the **beginning** of the scope (before non-declaration statements)?
- [ ] Is there a **blank line** after the last declaration before other statements (when both exist)?
- [ ] Are variables that are only needed inside a branch/loop declared in that inner scope (not forced to function top without need)?
- [ ] Are function-wide locals declared at function top rather than mid-body just before first use?
- [ ] Verdict: **ok** / **move to scope start** / **add blank line** / **narrow scope** (cite lines)

Initialization in the declaration (`int a = expr;`) is fine. Mixing declarations after executable statements in the same scope is not.

## Workflow

1. Read **Variable declarations** in `AGENTS.md`.
2. Resolve scope (named function vs file vs broader).
3. Walk each function/block; apply the checklist.
4. **If review only:** report (see output format). Do not edit.
5. **If refactor requested:** move declarations, insert blank lines, keep brace/`#endif`/other `AGENTS.md` rules; compile if a build exists.
6. Summarize fixes and deferred cases (e.g. RAII guards that must stay next to the resource for correctness — ask the user if unclear).

## Output format

```markdown
## Scope
[named function | files | broader]

## Findings
| Function / block | Issue | Evidence | Action |
|------------------|-------|----------|--------|
| ... | mid-scope decl / missing blank line / too wide | lines | move / blank line / narrow / keep |

## Changes
- [what was edited, or "review only — no edits"]

## Deferred
- [cases needing user confirmation]
```

## Named-function focus

When the user asks about one function:

1. Open its definition; list locals per nested scope.
2. Report each violation with before/after guidance.
3. If refactoring, change only that function unless a shared helper must move with it.
