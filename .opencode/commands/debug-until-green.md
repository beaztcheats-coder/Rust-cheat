---
description: Debug, fix, and verify until checks pass
agent: build
---

Debug and fix the issue below.

Use this loop:
1. Inspect the relevant code.
2. Identify the root cause before editing.
3. Make the smallest safe fix.
4. Run the relevant checks: tests, typecheck, lint, and build if available.
5. If any check fails, read the error carefully, fix the root cause, and rerun verification.
6. Continue until all relevant checks pass.
7. Do not make unrelated refactors.
8. Do not claim success unless verification was run.

If blocked by missing credentials, external services, or unavailable commands, explain exactly what blocked verification.

At the end, summarize:
- Root cause
- Files changed
- Commands run
- Final result

User request:
$ARGUMENTS
