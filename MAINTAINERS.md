---
title: Maintaining Sagan rendering
status: review-needed
publication_ready: false
verified_in: null
verified_on: null
verified_by: null
---

# Maintaining Sagan rendering

This is a public split repository, not a released package.
`libraries/render/sagan.toml` currently declares `sagan-render` version
`0.5.1`. Its repository-local `libraries/index.tsv` points to that manifest
and declares compiler compatibility `^4.0.0`. The source package remains
under `libraries/render/`; final root relocation is not yet complete.

## Exact focused Windows checks

Independent Windows CI checks out the exact language commit in
`sagan-source-commit.txt`, builds its compiler, and runs the two auto-closing
window tests below against this repository's `libraries/index.tsv`. The
native bridge has no validated Linux or macOS backend; a green Windows job
does not imply multi-platform rendering support.

Install or build compatible Sagan, MSYS2 UCRT64 g++, and a Windows desktop
session. From the candidate root in Git Bash on the current development
machine:

```bash
export SAGAN_EXECUTABLE=/c/Users/joeps/coding/sagan/bin/sagan.exe
bash tests/integration/window_bridge_test.sh
bash tests/integration/shape_text_test.sh
```

The first test opens and closes its own window after roughly 100 ms. The
second closes after roughly 150 ms and checks that its own generated BMP is
960×540. Both passed with the extracted package, current Sagan 4.9.5
development executable, and no monorepo launcher resource object. If Sagan
is correctly installed on PATH, omit `SAGAN_EXECUTABLE`; scripts default to
`sagan`. Set `SAGAN_PACKAGE_INDEX` explicitly only for a reviewed alternate
catalog; otherwise the candidate-local index is used. The scripts create
build output only under ignored `build/`. Do not kill another user-controlled
demo window while testing.

The corresponding manual demos are:

```bash
bash scripts/window_demo.sh
bash scripts/shape_text_demo.sh
```

These open native windows. The tests set `SAGAN_RENDER_AUTOCLOSE_MS` to avoid
leaving them open. The bridge uses an embedded custom icon if available and
a Windows stock icon otherwise. The latter is a compatibility fallback, not
a decision about final product branding. The current non-Windows backend
throws an explicit unsupported-platform error; do not claim Linux/macOS
rendering based on a passing Windows test.

## Branches and documentation

Inspect branch, HEAD, status, staged paths, compiler and native-toolchain
versions, and relevant history first. Every authorized request starts from
current `dev` on a fresh `codex/<request>` branch. Test the affected work,
refine until it works, commit only intended paths, merge into `dev`, and
rerun relevant tests after integration. Reserve the full suite for reviewed
`dev` to `main` promotion or release. Both promotion and releases are on
owner hold until explicitly reopened with publication/signing policy.

Structural-only documentation edits need not run unrelated executable
examples. Run affected examples when their content or supporting behavior
changes. Any edited documentation page resets to `status: review-needed`,
`publication_ready: false`, and null verification fields pending human audit.

## Extraction and recovery

The local pinned filter-repo rehearsal retained exactly 21 owned files and
19 relevant commits. Re-filter from final reviewed `dev`, verify exact paths
and `git fsck --full`, then publish only reviewed `dev` and `main` heads
without inherited Sagan language release tags. The independent package
root, platform CI, artifact packaging, official-docs aggregation, version
compatibility, and owner clean-machine drill remain incomplete. No split
remote was created or pushed in this rehearsal.

If a native build fails, distinguish compiler emission, package lookup,
resource loading, g++/linking, and runtime/display failures. A missing
custom icon must not make an otherwise valid window unusable, but a missing
or unsupported display backend is a real limitation. The current local-only
transfer backup cannot survive loss of this machine or recreate GitHub
settings, secrets, issues, or PR history.
