# Sagan rendering (local split candidate)

This is a local history-preserving rehearsal of `sagan-render`, not a
published independent repository. It contains the current Sagan rendering
package, native Windows bridge, two window demonstrations, focused tests,
and its technical page. The package remains under `libraries/render/` in
this first candidate; final root relocation and hosted CI are still open.

The candidate-local `libraries/index.tsv` resolves this extracted package.
The demo scripts accept `SAGAN_EXECUTABLE` or installed `sagan` and no longer
need the monorepo's launcher resource object. A bundled custom icon is used
when present; otherwise the bridge uses the Windows stock application icon.

Read [MAINTAINERS.md](MAINTAINERS.md) for exact Bash commands and recovery,
[TECHNOLOGY.md](TECHNOLOGY.md) for the architecture boundary, and
[CODEX_START.md](CODEX_START.md) for a new rendering chat.
