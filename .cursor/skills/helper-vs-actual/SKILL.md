---
name: helper-vs-actual
description: >-
  Classify C++ free functions as helpers or actual/reusable APIs, then review or
  refactor placement (anonymous namespace vs header/source). Also flags
  passthrough wrappers and trivial equality verifiers during function review.
  Use when the user asks to review helpers, promote/demote functions, audit
  reuse, refactor file-local helpers, or names a specific function to classify
  or move.
---

# Helper vs actual function review

Normative definitions live in [`AGENTS.md`](../../../AGENTS.md) under **Helper vs actual functions**, **Namespaces**, **Wrappers**, **Verifiers**, and **Variable declarations**. Do not invent parallel rules; apply those.

Related skills: [passthrough-wrappers](../passthrough-wrappers/SKILL.md), [trivial-verifiers](../trivial-verifiers/SKILL.md), [variable-declarations](../variable-declarations/SKILL.md).

## Scope modes

1. **Named function** — user names a symbol (e.g. `format_index`, `read_file`, `scripts_extract::extract`). Analyze only that function and its call sites unless they ask to expand.
2. **File / module review** — user points at a `.cpp`/`.hpp` or module. Classify every free function in that scope.
3. **Broader refactor** — user asks to fix misplacement across the tree. Start from the files they named; widen only with clear repeating patterns.

Default to the narrowest mode that answers the request. Prefer report-then-edit unless the user asked to refactor/apply.

## Classification checklist

For each candidate function:

- [ ] How many call sites? Which translation units?
- [ ] Is it declared in a header (public API) or only defined in a `.cpp`?
- [ ] Are parameters abstract/agnostic, or tied to one surrounding procedure?
- [ ] Does the same procedure or pattern already repeat elsewhere?
- [ ] Is it a **passthrough wrapper** (same parameter count/types forwarded unchanged to another function with no processing)? If yes, verdict is remove/inline — not “helper” or “actual”
- [ ] Is it a **trivial verifier** (body only returns whether a value equals/differs from an expected value)? If yes, verdict is remove/inline — not “helper” or “actual”. See [trivial-verifiers](../trivial-verifiers/SKILL.md)
- [ ] Verdict: **helper**, **actual**, **passthrough wrapper**, or **trivial verifier** (cite evidence)

### Verdicts and required placement

| Verdict | Placement |
|---------|-----------|
| Helper | Anonymous namespace in the owning `.cpp`; not in a header; no named `detail` namespace unless the user asks |
| Actual (module API) | Declared in the appropriate `.hpp`, implemented in `.cpp`; no pointless wrapping named namespace |
| Actual (extract from duplication) | Factor once with abstract parameters; update call sites; keep true one-off leftovers as helpers |
| Passthrough wrapper | Remove; call the target directly (or rename the target). See **Wrappers** in `AGENTS.md` |
| Trivial verifier | Remove; compare at the call site. See **Verifiers** in `AGENTS.md` |

Do **not** promote a helper to a header on speculation. Do **not** leave a clearly repeated agnostic pattern copy-pasted when it should become an actual function. Do **not** keep rename-only wrappers or equality-only verifiers.

## Workflow

1. Read the relevant **Helper vs actual** / **Namespaces** / **Wrappers** / **Verifiers** sections in `AGENTS.md`.
2. Resolve scope (named function vs file vs broader).
3. Gather evidence: definitions, declarations, call sites (`rg` / project search).
4. Classify with the checklist; note misplacements (bare file-scope helper, helper in header, actual buried as anonymous with external need, duplicated pattern not factored, passthrough wrappers, trivial verifiers).
5. **If review only:** report findings (see output format). Do not edit.
6. **If refactor requested:** apply the smallest placement/API change that matches the verdicts; keep brace/style/`#endif`/variable-declaration rules from `AGENTS.md`; rebuild or compile if the project already has a build.
7. Summarize what changed and what was left alone (and why).

## Output format

```markdown
## Scope
[named function | files | broader]

## Findings
| Symbol | Current placement | Verdict | Evidence | Action |
|--------|-------------------|---------|----------|--------|
| ... | anonymous / header / bare file scope | helper / actual / passthrough / trivial verifier | call sites, params, repetition, forward shape, equality-only body | keep / move / promote / demote / factor / remove |

## Changes
- [what was edited, or "review only — no edits"]

## Deferred
- [anything needing user confirmation, e.g. ambiguous reuse]
```

## Named-function focus

When the user asks about one function:

1. Locate its definition and any header declaration.
2. List all call sites.
3. Classify that symbol first (including passthrough and trivial-verifier checks); only mention neighbors if they block a correct move.
4. If they asked “should this be public?”, answer with helper vs actual evidence, not taste. If it is only a rename of another function or an equality-only verifier, say so and recommend removal.
