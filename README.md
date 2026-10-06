# Sagan rendering

This public history-preserving split repository contains the current Sagan rendering
package, native Windows bridge, two window demonstrations, focused tests,
and its technical page. The package remains under `libraries/render/` in
this first layout; final root relocation and hosted CI are still open.

Sagan Render is licensed under GPL-3.0-only. See [LICENSE.txt](LICENSE.txt) and
[NOTICE.txt](NOTICE.txt) for the complete terms and Schematic lineage.

The repository-local `libraries/index.tsv` resolves this extracted package.
The demo scripts accept an explicit `SAGAN_EXECUTABLE`. Without one, they read
`sagan-source-commit.txt`, reuse that exact compiler from the ignored `build/`
cache, or fetch and build it automatically. They deliberately do not trust an
unrelated `sagan` on PATH. A bundled custom icon is used when present;
otherwise the bridge uses the Windows stock application icon.

Read [MAINTAINERS.md](MAINTAINERS.md) for exact Bash commands and recovery,
[TECHNOLOGY.md](TECHNOLOGY.md) for the architecture boundary, and
`AGENTS.md` for lasting chat guidance.

The Windows-only dev-channel source package is reproducibly assembled with
`bash scripts/package-dev.sh`. Its extracted `libraries/index.tsv` is the
consumer entry point; see `MAINTAINERS.md` for clean-location verification and
rollback.
