---
name: trivial-verifiers
description: >-
  Find and remove C++ trivial verifiers that only return whether a value equals
  an expected value (equality renames of == / !=). Use when the user asks to
  review is_*/verify_*/check_*/expect_* helpers, audit equality predicates,
  remove type-tag façades, or names a specific function to check as a verifier.
---

# Trivial verifier review

Normative rule: [`AGENTS.md`](../../../AGENTS.md) under **Verifiers (no equality renames)**. Do not invent parallel rules.

Related skills: [passthrough-wrappers](../passthrough-wrappers/SKILL.md), [helper-vs-actual](../helper-vs-actual/SKILL.md).

Application code already dropped the marshal `is_*` type-tag methods; use this skill for future reviews or when new candidates appear.

## Scope modes

1. **Named function** — user names a symbol. Check whether it is a trivial equality verifier; analyze its body and call sites only unless they ask to expand.
2. **File / module review** — scan free functions (and obvious methods) in the named `.cpp`/`.hpp` for equality-only bodies.
3. **Broader refactor** — user asks to remove trivial verifiers across the tree; start from named files, then search for `is_*` / `verify_*` / `check_*` / `expect_*` and one-line `return … ==` / `!=` predicates.

Default to the narrowest mode. Prefer report-then-edit unless the user asked to refactor/apply.

## Detection checklist

For each candidate:

- [ ] Is the essential body a single comparison (`==` / `!=`) (or equivalent) that returns bool?
- [ ] Does it wrap a value, field, or public type/tag/enum the caller can already compare directly?
- [ ] Is there **no** multi-condition logic, range/set checks, parsing, normalization, or other non-trivial work?
- [ ] Verdict: **trivial verifier (remove/inline)** or **legitimate predicate (keep)**

### Verdicts

| Verdict | Action |
|---------|--------|
| Trivial verifier | Delete the function/method; update call sites to compare at the use site |
| Legitimate | Keep — multi-condition or other real predicate logic |
| Ambiguous | Report and ask; e.g. comparison plus a throw/message that might belong in a typed accessor |

A name like `is_*`, `verify_*`, `check_*`, or `expect_*` does not justify keeping an equality-only body. Type-tag façades over a public discriminant (`is_nil`, `is_array`, …) are trivial verifiers.

## Workflow

1. Read **Verifiers** in `AGENTS.md`.
2. Resolve scope (named function vs file vs broader).
3. Find candidates: bool functions/methods whose body is dominated by one equality/inequality.
4. Apply the checklist; record what is compared and whether callers can compare it directly.
5. **If review only:** report (see output format). Do not edit.
6. **If refactor requested:** remove trivial verifiers; inline comparisons at call sites; keep `AGENTS.md` style (including variable declarations at scope start + blank line after decls); compile if a build exists. When touching call sites or leftover helpers, prefer [variable-declarations](../variable-declarations/SKILL.md) if declaration placement is wrong.
7. Summarize removals and anything deferred.

## Output format

```markdown
## Scope
[named function | files | broader]

## Findings
| Symbol | Comparison | Caller can compare directly? | Non-trivial logic? | Verdict | Action |
|--------|------------|------------------------------|--------------------|---------|--------|
| ... | e.g. type == Type::Array | yes/no | yes/no | trivial / legitimate | remove / keep / ask |

## Changes
- [what was edited, or "review only — no edits"]

## Deferred
- [ambiguous cases needing user confirmation]
```

## Named-function focus

When the user asks about one function:

1. Open its definition; identify the comparison (if any).
2. Check whether callers already have access to the compared value/tag.
3. Say clearly whether it is a forbidden trivial verifier or an accepted predicate, with a short cite of the body.
4. If removing, list call sites that must switch to an inline comparison.
