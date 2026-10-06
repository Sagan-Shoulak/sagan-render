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

The package-facing canvas API still uses the temporary Windows bridge. The
private SDL foundation now validates both native windowing and a first GPU
frame on Windows, Linux, and macOS. It is the implementation seam beneath the
future public API; SDL window, device, command, texture, and pipeline handles
remain private. Linux CI runs under Xvfb with Mesa's Vulkan implementation, so
it proves X11/Vulkan lifecycle and pixels in a virtual display rather than a
physical desktop. macOS CI exercises Cocoa and Metal in its runner session.
The dependency direction is rendering to Sagan's language/math/toolchain;
physics and the Space Game may consume rendering but do not own its native
bridge. The future workspace exact lock coordinates tested versions.

Issue #15 begins a cross-platform rebaseline around an SDL3 window
layer and SDL GPU rendering layer. The accepted design preserves programmable
shaders, lighting, post-processing, and compute as intended capabilities while
keeping backend objects private. The window probe uses an SDL software surface
to isolate window behavior. The GPU probe separately creates a real GPU device
and renders through D3D12, Vulkan, or Metal. See
[the backend rebaseline](docs/backend-rebaseline.md).

## How the first GPU frame works

The CPU does not execute a draw call immediately. It records ordered work in an
SDL GPU command buffer. A render pass begins by clearing a 256×256 RGBA texture,
then binds a graphics pipeline and asks for three vertices. The vertex shader
uses the built-in vertex index, so this first triangle needs no vertex buffer;
it emits one position and color for each corner. The fragment shader
interpolates those colors across the triangle.

After the render pass ends, the same command buffer blits the offscreen texture
to the window's swapchain texture. The swapchain is the platform-managed chain
of presentable images. Keeping deterministic validation in a separate
offscreen texture avoids depending on desktop composition, scaling, or capture
timing. A copy pass downloads that texture into a transfer buffer. A fence lets
the CPU wait until the GPU has finished before inspecting or saving those
bytes.

The resulting ownership flow is:

```text
SDL window -> GPU device -> command buffer -> render pass -> offscreen texture
                                               |                 |
                                               |                 +-> transfer buffer -> BMP/test
                                               +-> pipeline -> triangle
offscreen texture -> blit -> swapchain texture -> desktop
```

Destruction runs in the reverse direction after the device is idle: pipeline,
transfer buffer, texture, claimed window, device, then window. This order keeps
resources from outliving the device that owns them.

## UI layout, physical views, and pixels

The UI contract uses logical display units. Rows and columns first measure
minimum, preferred, and maximum sizes, distribute remaining space by grow
weight, and then arrange their children inside padding and gaps. Overlays share
the same logical coordinate space and are hit-tested in reverse paint order, so
the last painted eligible control receives the pointer. Parent and viewport
rectangles clip painting and hit regions before anything reaches a backend.

There are three intentionally separate length domains:

```text
physical length with a Sagan unit subtype
  -> orthographic camera scale
logical view or UI length
  -> operating-system display scale
drawable pixels
```

For example, a camera may declare that its horizontal span is exactly
`400000 kilometer`. The camera derives its vertical physical span from the
logical viewport aspect ratio, maps those physical coordinates into the view,
and leaves display scaling to the final conversion. Moving a window between a
1x and 2x display therefore doubles its drawable pixels without changing the
400,000 km represented by the scene or the logical size of a UI button.

The native contract models physical coordinates with distinct wrapper types so
that pixel coordinates cannot be passed accidentally. The future public Sagan
scene/camera API must be stronger: it will accept Sagan numeric values with a
length unit subtype and use explicit conversions for display and physical
units. The existing temporary `canvas.set_view` API still takes bare scalars;
do not treat it as the final unit-safe contract. Camera ownership and the
public measured API remain part of the scene/camera work tracked under #12 and
#20, while #17 establishes the separation required by UI layout.

Text follows a similar boundary. Layout sends text, font family, point size,
wrap width, direction, and language to a shaping interface. A shaper returns
glyph identifiers, advances, offsets, and logical bounds. Backend font and
glyph resources remain private and can later be cached without changing the
layout contract.

The frame order is:

```text
content/style change -> measure -> arrange -> clip -> paint list -> rasterize
drawable input -> logical coordinates -> topmost hit test -> focus/capture -> action
```

Window logical-size or display-scale changes invalidate arrangement and
rasterization; content, font, and style changes also invalidate measurement.
Relayout identifies controls by stable IDs, preserves keyboard focus when the
same enabled control survives, and cancels pointer capture so a removed or
moved control cannot receive a stale release. Activation occurs only when a
captured pointer is released inside the still-enabled control. Cleanup clears
focus, capture, and pending actions before renderer resources are destroyed.

### GPU UI demonstration

The UI demo is authored in Sagan. Its program owns responsive layout, focus and
activation state, pointer hit-testing, and the unit-typed camera calculations.
The private native bridge turns its backend-neutral commands into real SDL GPU
work without leaking SDL objects into Sagan. Each fill command contains only
logical bounds and an RGBA color. The GPU compositor owns a tiny palette
texture, a logical-size offscreen target, a download buffer used only for
evidence, and the claimed window. It blits palette texels into clipped
rectangles, then blits the completed offscreen target to the current swapchain
size. Static UI is repainted only when state changes; ordinary frames reuse the
offscreen target and perform one presentation blit.

Interactive resize reallocates that target to the current logical window size
and reruns Sagan layout. Text therefore retains its logical height and aspect
instead of stretching with an old frame. The final blit accounts for drawable
density, whose aspect matches the logical window. CI launches the same Sagan
program at 960×540 and 800×600. This avoids depending on a headless window
manager accepting a programmatic resize while exercising the same size-query,
target-allocation, and Sagan relayout path used by real resize events.

The demonstration font implements the `text_shaper` boundary with fixed 5×7
glyph data. Measurement produces logical advances and painting emits glyph
cells into the same draw list as panels and buttons. This small font makes
cross-platform reference pixels deterministic; it is not the final Unicode,
script-shaping, fallback, or accessibility font system. Replacing it does not
change layout or the GPU consumer because both depend on the shaping
request/result contract rather than a platform font handle.

Every captured demo frame must contain minimum counts of exact white text and
Earth pixels. That catches a blank frame, absent text, or lost scene content
before the BMP is accepted. The same emitted Sagan program runs against D3D12,
Vulkan, and Metal in CI. Individual rectangle blits are a portable bootstrap; a
later shader-backed batch may consume the same draw list without changing UI
descriptions.

The measured scene uses two explicit orthographic transforms in Sagan. Its main
view is 200,000,000 km wide, placing the Sun and Earth from values typed with
the `meter` unit and deriving a 50,000,000 km scale bar as a ratio of physical
span to logical width. A separate
1,000,000 km Earth–Moon inset makes the 384,400 km separation legible without
falsifying the Solar-scale positions. Resizing changes both bars' pixel lengths
through the camera transform while their declared physical lengths remain
constant.

The public `render.ui` facade is Sagan source. Its compiler-recognized private
bridge names are pinned through `sagan-source-commit.txt`; they are package
plumbing rather than public language built-ins. C++ consumes draw/event data,
rasters the deterministic bootstrap font, and owns SDL/GPU resources. It does
not choose scene positions, physical spans, control geometry, or UI state.
Until Sagan-Shoulak/sagan#8 removes redundant equality parentheses from emitted
C++, the portable demo build suppresses only Clang's
`-Wparentheses-equality`; all other Clang diagnostics remain fatal.

### Loading-screen example

`examples/loading_sagan_demo` uses the same Sagan UI facade and native GPU host
to demonstrate four renderer-facing presentation states: loading progress,
ready, failure, and an animated transition. The centered card and progress bar
are recomputed from current logical dimensions, so drawable density and window
aspect do not stretch stored pixels.

The example intentionally uses a deterministic synthetic loading driver. It
stands in for application-owned asset work without defining game policy or
pretending that this renderer package owns an application asset graph. The
renderer is responsible for displaying the supplied state; a real application
will replace the timer and F/R controls with its loader's progress and error
signals. CI freezes the native monotonic clock and injects an input event to
capture loading, ready, and asset-failure presentations deterministically.

The transition is a Sagan-authored horizontal wipe between three and four
seconds. Native code exposes only elapsed monotonic time; it does not choose
state thresholds or geometry. Closing from any state destroys the draw list,
GPU target, palette, transfer buffer, device claim, and window through the same
bridge cleanup path.

## Shader formats and temporary bootstrap assets

SDL GPU selects a native desktop backend, not a universal shader bytecode.
The same reviewed HLSL behavior is represented as DXIL for D3D12, SPIR-V for
Vulkan, and MSL for Metal. The probe chooses the format reported by its device
and uses backend-specific entry-point metadata while keeping those choices out
of the future public API.

For #15, the compiled fixtures are pinned to the zlib-licensed SDL GPU examples
commit recorded in `third_party/sdl-gpu-foundation-shaders.lock`. Every download
is SHA-256 checked. This is intentionally a bootstrap path, not the final asset
pipeline. Issue #16 will own reproducible project shader compilation,
reflection, materials, color policy, and lighting.

## Supported foundation and rollback

- Windows validation explicitly requests SDL's `direct3d12` driver and DXIL.
  SDL 3.4.16 documents Windows 10+, DirectX 12 feature level 11_0, and resource
  binding tier 2 as the normal requirements.
- Linux validation explicitly requests `vulkan` and SPIR-V. CI uses Mesa's
  software Vulkan path under Xvfb; real distributions still need a conforming
  Vulkan driver and SDL's documented required extensions.
- macOS validation explicitly requests `metal` and MSL. SDL 3.4.16 documents
  macOS 10.14+ with Apple Silicon or the supported Intel Mac GPU family.

The existing Win32 bridge remains the rollback path while the public package
is migrated. Reverting the SDL foundation commits removes its tests and pinned
fixtures without changing the current Sagan canvas behavior. Do not delete the
Win32 bridge until the unified API and package have independently passed their
replacement gates.

SDL is pinned to release 3.4.16, source commit
`fa2c02bb6e21974a89ea9824bc53c9932abe5f9c`, with release-asset checksums in
`third_party/sdl3.lock`. Windows consumes the official MinGW development
archive. Linux and macOS CI build the official source archive at the same
version. Backend and platform limitations must continue to be reported
explicitly.

The local Windows extraction tests passed the short auto-closing window check,
the shape/text BMP capture check, the SDL window lifecycle/capture probe, and
the D3D12/DXIL triangle readback. Cross-platform status is established by the
named CI jobs and their BMP artifacts, not inferred from the Windows result.
See [MAINTAINERS.md](MAINTAINERS.md).
