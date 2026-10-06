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

The package-facing native backend still supports Windows only. The
foundation probe now separately validates that pinned SDL3 can create,
paint, capture, and destroy a window on Windows, Linux, and macOS. This probe
is evidence for the selected cross-platform window dependency; it is not yet
wired into the public Sagan API. Linux CI runs under Xvfb, so it proves the X11
lifecycle and pixels in a virtual display rather than a physical desktop.
macOS CI exercises the Cocoa window lifecycle in its runner session.
The dependency direction is rendering to Sagan's language/math/toolchain;
physics and the Space Game may consume rendering but do not own its native
bridge. The future workspace exact lock coordinates tested versions.

Issue #15 begins a cross-platform rebaseline around an SDL3 window
layer and SDL GPU rendering layer. The proposal preserves programmable shaders,
lighting, post-processing, and compute as intended capabilities while keeping
backend objects private. The window probe uses an SDL software surface only to
make the first platform test observable; it does not choose that surface API as
the eventual renderer. GPU device creation and the teaching-led rendering
pipeline remain later slices. See
[the backend rebaseline](docs/backend-rebaseline.md).

SDL is pinned to release 3.4.16, source commit
`fa2c02bb6e21974a89ea9824bc53c9932abe5f9c`, with release-asset checksums in
`third_party/sdl3.lock`. Windows consumes the official MinGW development
archive. Linux and macOS CI build the official source archive at the same
version. Backend and platform limitations must continue to be reported
explicitly.

The local Windows extraction tests passed the short auto-closing window check,
the shape/text BMP capture check, and the new SDL window lifecycle/capture
probe. Cross-platform status is established by the named CI jobs and their BMP
artifacts, not inferred from the Windows result. See [MAINTAINERS.md](MAINTAINERS.md).
