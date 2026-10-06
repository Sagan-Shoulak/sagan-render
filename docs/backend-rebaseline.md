---
title: Renderer backend rebaseline
status: review-needed
publication_ready: false
verified_in: null
verified_on: null
verified_by: null
decision: accepted
---

# Renderer backend rebaseline

Issue #10 requires one rendering API with consistent Windows, Linux, and macOS
behavior. The current Win32/GDI bridge cannot satisfy that contract and does
not provide a scalable path to programmable shaders or lighting. This document
defines the accepted boundary. M0 through M2 have executable foundation
evidence; later renderer and UI milestones remain planned.

## Accepted stack

- SDL3 owns windows, platform events, input focus, display changes, and DPI
  discovery.
- SDL GPU owns graphics devices, command buffers, swapchains, textures,
  samplers, buffers, render passes, compute passes, and graphics pipelines.
- SDL_shadercross compiles project-owned shaders ahead of time into the formats
  required by the selected SDL GPU backend.
- Sagan-facing APIs remain backend-neutral. SDL handles and GPU objects stay in
  the private native bridge.

SDL GPU maps the desktop platforms to D3D12, Vulkan, and Metal. This preserves
a path to vertex and fragment shaders, post-processing, materials, 2D and 3D
lighting, and compute without exposing those platform APIs to Sagan programs.
The first implementation will deliberately use only the shared feature subset;
backend-specific effects require a later capability policy.

Primary references:

- <https://wiki.libsdl.org/SDL3/SDL_CreateWindow>
- <https://wiki.libsdl.org/SDL3/README-highdpi>
- <https://wiki.libsdl.org/SDL3/CategoryGPU>
- <https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader>

## Cross-platform behavior contract

The public API distinguishes three coordinate spaces:

1. Logical UI units describe layout and input hit testing.
2. Drawable pixels describe render targets and captures.
3. World units describe simulation-facing geometry before camera projection.

The backend reports logical window size, drawable pixel size, content scale,
focus, and close state independently. UI layout uses logical units. Raster work
uses drawable pixels. Pointer events are normalized into logical units before
they reach controls. A resize or display-scale event invalidates layout and
recreates only size-dependent GPU resources.

The renderer defines its own color, blending, sampling, depth, winding, and
matrix conventions. Tests compare those contracts rather than assuming that
D3D12, Vulkan, and Metal defaults match. Resource creation and destruction are
explicit, deterministic, and validated while the graphics device is alive.

## Milestones

### M0: native window seam — implemented

Agent-owned implementation work:

- introduce a private platform/window interface;
- move the current Win32 implementation behind it without changing public
  behavior;
- define logical size, drawable size, scale, focus, resize, and close events;
- add lifecycle and event-harness tests.

Completion test: the existing Windows examples remain byte-for-byte compatible
where deterministic, and the window contract can be exercised without canvas
drawing.

### M1: SDL3 window backend — implemented foundation

Agent-owned native integration work:

- pin and build SDL3 reproducibly;
- implement the M0 window contract with high-pixel-density support;
- add Windows, Linux, and macOS CI smoke tests;
- retain the Win32 bridge only as a temporary rollback path.

Completion test: one source-level window example opens, resizes, reports scale,
processes focus and close events, and cleans up on all three platforms.

### M2: first GPU frame — implemented foundation

Implementation and teaching evidence:

- learn the device, command-buffer, swapchain, render-pass, pipeline, and shader
  lifecycle;
- render a clear color and one triangle through the backend-neutral contract;
- load one checksummed shader set for D3D12, Vulkan, and Metal;
- capture and validate a deterministic offscreen frame.

Completion test: the same scene and public calls produce equivalent output on
all supported backends.

The focused probe creates a native SDL window and an explicitly selected GPU
device: D3D12 on Windows, Vulkan on Linux, and Metal on macOS. It records one
command buffer containing an offscreen render pass, a bufferless RGB triangle,
a blit to the window swapchain, and a download to CPU-visible memory. The test
checks the returned pixels for substantial red-, green-, and blue-dominant
regions and retains a 256×256 BMP. This validates the private first-frame seam;
the Sagan-facing API will be composed over it without exposing SDL objects.

The temporary compiled shaders come from the zlib-licensed SDL GPU examples at
the commit and checksums in `third_party/sdl-gpu-foundation-shaders.lock`.
Issue #16 replaces this bootstrap dependency with the reviewed project-owned
shader compilation pipeline.

### M3: scalable 2D foundation

Agent-led implementation with explanation and comprehension evidence:

- batch textured and solid-color geometry;
- establish orthographic cameras, scissor rectangles, alpha blending, texture
  atlases, and font resources;
- keep immediate UI descriptions separate from retained GPU resources.

Completion test: a resize-safe loading screen renders text, progress, imagery,
and animated transitions without per-widget draw calls.

### M4: UI layout and controls

Agent-led implementation with explanation and comprehension evidence:

- implement rows, columns, overlays, constraints, padding, alignment, and scale;
- add toolbar buttons, focus traversal, pointer capture, keyboard activation,
  and accessibility-ready semantic roles;
- keep UI state independent from game-specific presentation.

Completion test: inspectable loading-screen and toolbar examples pass input,
resize, DPI, focus, and cleanup harnesses on all three platforms.

### M5: effects and lighting path

Agent-led implementation with explanation and comprehension evidence:

- define material and uniform interfaces without exposing backend handles;
- add a post-processing pass and a small 2D lighting example;
- document shader inputs, color space, precision, and fallback behavior.

Completion test: a shader-based effect and modest multi-light scene behave
consistently across D3D12, Vulkan, and Metal.

The first M5 contract slice is implemented: a project-owned HLSL mesh shader,
backend-neutral material/light uniform layouts, linear-color conversion, and a
CPU reference Lambert calculation. It deliberately stops before claiming an
SDL GPU pipeline: reproducible SDL_shadercross artifacts and actual GPU use are
the next gate.

## Deliberate non-decisions

This proposal does not yet select a scene graph, entity system, UI styling
language, font-shaping library, asset format, or physically based rendering
model. It also does not promise every feature of every GPU. Those choices need
small executable examples and measured constraints before becoming contracts.
