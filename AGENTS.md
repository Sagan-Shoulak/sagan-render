# Rendering repository agent rules

Read `CODEX_START.md` once if present, then `TECHNOLOGY.md` and
`MAINTAINERS.md`. The first chat removes the tracked bootstrap prompt through
a PR; do not recreate it. These guides and the versioned ecosystem/chat maps
remain durable. Keep renderer APIs and native bridges here; consume language/math
and physics packages without copying their implementations. Make backend
and platform limitations explicit.

Use Bash commands for the owner, never PowerShell. Preserve unrelated work.
Follow the branch, focused-test, `dev` integration, full-suite-on-`main`,
documentation-review reset, and release-hold rules in `MAINTAINERS.md`.
The owner prefers to be taught what to write; implement only when asked.

Use issues for substantive work, linked PRs into `dev`, and the organization
Project for cross-repo milestones when accessible. Record focused native tests,
dependency pins, and integration impact; track blocked Project access in the
issue.

Route by owner: `sagan` language/toolchain, `sagan-vscode` editor,
`sagan-physics` numeric physics, `sagan-render` rendering,
`sagan-workspace` exact-lock integration, `sagan-docs` official site, and
`sagan-space-game` application/design. Handoffs to another chat must include
goal, evidence, constraints, pins, and verification.
