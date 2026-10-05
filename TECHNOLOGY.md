---
title: Sagan rendering technology overview
status: review-needed
publication_ready: false
verified_in: null
verified_on: null
verified_by: null
---

# Sagan rendering technology overview

This repository owns rendering APIs and native platform bridges, not Sagan
syntax, orbital physics, or game rules. The Sagan package under
`libraries/render/` exposes window and canvas operations. The Windows bridge
in `libraries/render/native/window_bridge.cpp` connects generated native
code to Win32 window creation, drawing, event polling, text, and optional
frame capture. The two current demonstrations exercise a basic window and a
static shape/text composition.

`libraries/render/sagan.toml` declares package identity and version.
`libraries/index.tsv` is a candidate-local catalog for testing this source
without accidentally loading the monorepo copy. The scripts ask an
installed or explicitly selected Sagan compiler to emit package-linked C++
for each demo, then compile it with the bridge. A custom icon resource is
used when the executable bundles one; the bridge falls back to a stock
application icon when it does not. This removes a hidden dependency on the
language repository's launcher resource object while preserving branding
where available.

The current native backend supports Windows only. The non-Windows path
reports that limitation rather than silently claiming a window backend.
Linux and macOS backend design, display-server integration, packaging,
headless test strategy, and release compatibility remain separate work.
The dependency direction is rendering to Sagan's language/math/toolchain;
physics and the Space Game may consume rendering but do not own its native
bridge. The future workspace exact lock coordinates tested versions.

The local Windows extraction tests passed the short auto-closing window
check and the shape/text BMP capture check without the monorepo resource
object. This is focused evidence for the candidate's current Win32 path,
not proof of all backends or display environments. See [MAINTAINERS.md](MAINTAINERS.md).
