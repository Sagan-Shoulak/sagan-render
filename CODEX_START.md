# Starting a new Sagan rendering chat

Begin read-only. Read `AGENTS.md`, `TECHNOLOGY.md`, `MAINTAINERS.md`,
`README.md`, `libraries/render/sagan.toml`, the rendering technical page,
the bridge source, examples/tests, and the versioned ecosystem/chat maps
from the primary repository. Inspect branch, HEAD, status, staged paths,
compiler version, native toolchain, package catalog, platform, and concurrent
work. State that this is a public split repository with a Windows-only backend,
not an independently released package.

Prefer teaching me what to code through small steps, API sketches, examples,
backend tradeoffs, review, and verification. Implement rendering code only
when I explicitly request it. For authorized changes, use a fresh
`codex/<request>` branch from current `dev`, test affected work until it
passes, commit only intended paths, merge to `dev`, and rerun relevant tests.
Reserve the full suite for `main` promotion or release; both remain on owner
hold. Do not run unrelated documentation examples for a structural-only
edit. Reset every edited documentation page to `review-needed`,
`publication_ready: false`, and null verification metadata.

Use the chat map to recommend the language, physics, game, workspace, or
official-docs chat for work they own. Supply a self-contained ready-to-paste
handoff with goal, evidence, constraints, dependency pins, and verification;
do not assume shared chat history. Use Bash, never PowerShell. Preserve
unrelated work and do not push, publish, release, transfer, or change remote
settings without current authorization.

This tracked prompt is a one-time bootstrap. After reading it and orienting
read-only, delete `CODEX_START.md` on a short-lived branch, commit that
deletion and any required contract updates, then open a PR into `dev` linked
to an onboarding issue. Do not
recreate it; `AGENTS.md`, `TECHNOLOGY.md`, and `MAINTAINERS.md` remain the
durable instructions.

Use existing or new GitHub issues for substantive work, PRs into `dev` for
review, and the organization Project for cross-repo milestones when access
permits. Link each PR to its issue, record focused native/window tests,
dependency pins, and integration impact, and update Project status. If
Project access is unavailable, record that in the issue and continue safe
local verification. The split is tracked by Sagan-Shoulak/sagan#6.
