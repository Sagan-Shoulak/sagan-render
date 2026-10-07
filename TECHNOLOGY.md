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

Manual demo launchers resolve the source pin through one shared script. An
explicit `SAGAN_EXECUTABLE` is authoritative; otherwise the resolver uses an
exact-pin compiler cache or builds the pinned Sagan checkout. It does not fall
back to an unrelated executable on PATH, since an older compiler can reject
private bridge names before package code is emitted. Compiler source and build
products remain ignored dependencies under `build/`, not copied language
implementation in this rendering package.

## Scene snapshots, cameras, and precision

`render.scene` is deliberately physics-agnostic. Its `SpatialRenderable` face
promises only a stable identifier, a three-dimensional position, a display
radius, and a label. An application may implement that face on a
presentation-facing class, or use a small adapter around a physics-owned
snapshot. The `sample` function copies only those fields into an immutable
`RenderSample`. The renderer does not know about mass, velocity, acceleration,
forces, timesteps, orbital elements, integrators, or mutable solver objects.

This boundary permits physics-forward classes to be made visible without
moving their behavior into the renderer. It also avoids forcing
`sagan-physics` to depend on `sagan-render`: an application package that already
depends on both can own the adapter. A demo may synthesize positions to explain
the camera, but such motion is example policy and not a renderer API.

Positions and radii cross the public Sagan boundary as `Float64<meter>`.
Logical viewport dimensions remain unitless display coordinates, and drawable
pixels remain private to the native backend. A frame is prepared in this order:

```text
physics/application immutable snapshot
  -> application-owned SpatialRenderable adapter
  -> renderer RenderSample values in physical units
  -> subtract camera origin in Float64<meter>
  -> rotate into camera axes
  -> perspective projection and frustum visibility
  -> logical coordinates and stable label anchors
  -> backend-specific GPU representation
```

Origin subtraction must occur before narrowing values for GPU buffers. For
example, two objects 32 metres apart near an absolute coordinate of
`1e15 meter` remain distinguishable after their shared nearby origin is
subtracted; converting both absolute positions to 32-bit floats first would
erase that separation. Moving the camera or choosing another rendering origin
creates different frame data and never mutates the source snapshot.

The initial camera contract uses a right-handed world. A default camera looks
along negative world Z with positive Y up. The native deterministic seam builds
an orthonormal right/up/forward basis. Its row-major view matrix operates on
column vectors and contains rotation only because translation has already been
handled by origin subtraction. The backend-neutral perspective matrix maps X
and Y to `[-1, 1]` and forward depth to `[0, 1]`; a D3D12, Vulkan, or Metal
adapter may transpose matrices or use reverse depth privately.

Near/far distances and object radii use metres. Sphere/frustum tests include an
object whose center is just outside a plane when its radius still intersects
the view. `linear_depth` is a stable renderer-facing ordering and inspection
value, not a promise about the nonlinear value ultimately stored in a hardware
depth buffer. Precision limits, clipping policy, and matrices are tested in
`tests/integration/scene_contract_test.sh`, which also emits the inspectable
`build/scene-contract/scene-camera.svg` artifact on every supported platform.
The Sagan-authored `examples/scene_sagan_demo` sends three presentation
snapshots through the existing GPU UI host at coordinates near `1e15 meter`.
Cross-platform focused tests require the initial and later captures to differ
on D3D12, Vulkan, and Metal. The demo advances its snapshots; no sample
position is advanced by the renderer.

### Selection, focus, and spatial labels

Selection owns a stable non-zero render identifier, never a reference to an
application or physics object. Picking examines the current frame's visible
projected spheres in logical coordinates. When hit regions overlap, the nearest
linear depth wins; an identifier tie-break makes the result deterministic.
Replacing a snapshot therefore cannot leave an object pointer dangling, and a
missing identifier can be cleared without consulting a solver.

A focus request derives a target camera position from a render sample, the
camera's normalized forward direction, and an explicitly measured viewing
distance. `focus_transition` interpolates from the current camera position with
a cubic smoothstep. It subtracts endpoints before scaling the delta, which
keeps the transition stable around large coordinates. Only camera state
changes; render samples and upstream snapshots remain immutable. The public
Sagan `smooth_focus_axis` exposes the same rule for application composition.

Spatial labels begin with stable projected anchors. The deterministic placement
pass sorts nearer labels first, uses identifiers as its tie-break, clamps labels
to the logical viewport, and moves later overlapping labels vertically. It does
not alter object positions and remains independent of drawable-pixel density.
Production text shaping may change measured label bounds without changing this
placement contract.

The interactive scene demo uses Left and Right to select every sample, Enter to
start a focus transition, R to restore the system framing, and pointer clicks
to pick projected samples. Focusing interpolates all three measured camera
axes, then follows the selected object's fresh presentation snapshots. The
Sun, Earth, and Moon viewing distances are demo-owned presentation choices;
the generic scene library neither selects them nor changes a body position.

### Sun-Earth-Moon bootstrap

The first #29 slice replaces generic scene markers with unit-typed Sun, Earth,
and Moon fixture data. It uses mean radii, a 149,597,870.7 km Sun-Earth
separation, and a 384,400 km Earth-Moon separation. These values are deterministic
demo inputs, not an ephemeris or an orbital solver. At the full-system camera
scale the Moon marker is offset as a labeled callout because its true projected
separation from Earth is smaller than the minimum selectable marker size; the
underlying physical coordinate is not altered. The callout disappears at
closer framing, where the measured separation can be shown directly.

`render.bodies.draw_sphere_impostor` builds a shaded spherical silhouette from
GPU-composited horizontal strips. The smaller, upper-left lit silhouette acts
as a coarse Lambert-like light response over a darker full disc. This makes
depth and body curvature inspectable through the existing cross-platform GPU
path, but it is explicitly a bootstrap rather than a triangle-mesh material.
Issue #16 still owns shader compilation, mesh/material inputs, linear color,
and the final lighting pipeline. Replacing the impostor must not change stable
IDs, unit-typed positions/radii, camera focus, or snapshot ownership.

The demonstration advances an Earth phase and a faster Moon phase with a
bounded Taylor approximation of sine and cosine, then constructs fresh
presentation objects for the frame. This is intentionally bare-bones circular
motion, not an orbital integrator: it has no masses, forces, energy model,
ephemeris, error control, or persistent mutable body state. A later application
can replace the generator with versioned `sagan-physics` snapshots without
changing renderer APIs.

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

### Reusable controls and toolbars

`render.controls` implements reusable button state and toolbar focus in Sagan.
`ButtonState` owns enabled, hovered, pressed, focused, and pending-activation
state. Pointer release activates only when a press began on an enabled control
and the pointer is still inside; keyboard activation uses the same pending
action. Taking an action clears it, and relayout cleanup cancels hover and press
without discarding focus.

`ToolbarFocus` owns a stable numeric focus position and wraparound traversal.
The example supplies the enabled-control policy, skipping its disabled entry in
both directions, then synchronizes each button's focused state. This keeps the
generic focus object independent of a particular toolbar's labels or actions.

The toolbar example uses the same controls in a horizontal layout at ordinary
widths and a vertical layout below 760 logical units. Bounds are recomputed
before hit testing on every frame, and canvas clipping remains in the shared
draw-list backend. Native code reports pointer motion and button edges but does
not decide hover, capture, focus, activation, colors, spacing, or orientation.

CI proves behavioral equivalence by comparing complete captures: Enter and a
pointer click on Primary must be byte-identical, while a disabled click must be
byte-identical to idle. Forward and reverse traversal must reach the same
enabled control. Hover and press must each differ from their preceding state,
and the 640×600 capture must show the vertical layout.

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

### First project-owned material contract

`shaders/material/lit_mesh.hlsl` is the first renderer-owned 3D shader source.
Its vertex stage accepts position and normal attributes plus an already-built
model/view/projection transform. Its fragment stage accepts material and
lighting uniforms. It does not receive mass, velocity, orbital elements, or
other simulation state.

The material separates linear base color, linear emissive color, and roughness.
The initial shader uses ambient plus one clamped Lambert diffuse term; roughness
is reserved in the stable layout but is not interpreted until the chosen
specular model is reviewed. Light direction, color, and intensity are frame
inputs rather than properties of a physics body. This keeps a future Sun light
or application-authored lights outside the renderer's scene/snapshot contract.

CPU and HLSL layouts use 16-byte groups compatible with SDL GPU's std140
uniform-data requirement. The vertex uniform occupies `b0, space1`; fragment
material and lighting occupy consecutive `b0` and `b1` registers in `space3`.
The companion manifest records entry points, vertex semantics, and resource
counts needed by `SDL_GPUShaderCreateInfo`. `material_contract_test.sh` checks
the byte layout, sRGB-to-linear conversion, reference Lambert calculation, and
SDL binding convention on every supported platform.

This slice defines and tests the source interface only. Ahead-of-time
SDL_shadercross compilation, reflected artifact validation, GPU pipeline use,
textures, multiple lights, post-processing, and replacement of the sphere
impostor remain open under #16. The repository must not claim that handwritten
source plus a manifest is equivalent to compiled DXIL, SPIR-V, or MSL.

The first shader-toolchain dependency boundary is now pinned in
`third_party/sdl-shadercross.lock`. `fetch-shadercross-source.sh` checks out
that exact upstream commit into ignored build storage and rejects a cache whose
HEAD differs. This initial fetch intentionally leaves submodules uninitialized:
the next reviewed chunk must pin and inspect the much larger DXC, SPIRV-Tools,
SPIRV-Headers, and SPIRV-Cross dependency graph before any build begins.

That graph is now explicit in the lock. SDL_shadercross directly pins
SPIRV-Cross, SPIRV-Headers, SPIRV-Tools, and SDL's DirectXShaderCompiler fork.
The DXC fork in turn pins its own SPIRV-Headers, SPIRV-Tools, and
DirectX-Headers revisions; its `.gitmodules` mentions googletest but the pinned
tree has no googletest gitlink. The two SPIR-V revision pairs intentionally
differ and must not be collapsed merely because their repository names match.
Source fetch remains non-recursive until the build chunk decides exactly which
vendored targets are necessary and records their license/install impact.

The reviewed build plan is now recorded in
`third_party/shader-toolchain-build.lock` and explained in
`docs/shader-toolchain.md`. One pinned Linux x86-64 generator will build a
static SDL_shadercross CLI against separately provided SDL and SPIRV-Cross and
the checksummed official DXC `v1.9.2602` Linux binary package. It will produce
DXIL, SPIR-V, and MSL ahead of time; ordinary Windows, Linux, and macOS renderer
builds consume those artifacts and do not require a shader compiler.

The selected build initializes only the pinned SPIRV-Cross source gitlink. It
does not build the vendored SPIRV-Headers, SPIRV-Tools, or DXC source graph,
and it does not install or publish compiler libraries or executables. SDL and
SDL_shadercross are zlib licensed; SPIRV-Cross files use Apache-2.0 OR MIT;
the exact DXC Linux archive retains both `LICENSE-LLVM.txt` and
`LICENSE-MS.txt`. Any future redistribution of these tools requires a separate
license review.

The Linux generator now implements that plan in
`scripts/build-shader-toolchain-linux.sh`. It cross-checks all three source
pins, verifies the DXC archive digest before extraction, initializes only the
SPIRV-Cross gitlink, builds each dependency into ignored isolated directories,
and smoke-tests the resulting CLI. Linux CI performs the real build; the
cross-platform plan test separately prevents configuration drift on every
runner. This still does not replace the bootstrap runtime shaders: project
artifact generation and reflection checks are the next boundary.

The material artifact generator compiles `VSMain` and `PSMain` from that one
HLSL source into DXIL, SPIR-V, MSL, and reflection JSON. The focused test checks
container magic, textual MSL stages, vertex inputs, uniform-buffer counts, and
source/tool provenance. CI runs the generation twice and rejects any byte
difference, then retains one set as inspectable evidence. Runtime pipelines do
not consume these files yet; generation proof and backend integration remain
separate review boundaries.

The #29 completion target is a full 3D scene, not a more elaborate impostor.
Sun, Earth, and Moon must be model-based entities backed by vertex/index
geometry, transformed and depth-tested by the shared 3D pipeline. Its
interactive perspective camera will expose yaw, pitch, translation or dolly,
and focus/orbit behavior while remaining horizon locked: camera right and
forward are rebuilt from a declared world-up axis, roll is not an input, and
pitch is clamped before forward becomes parallel to world up. The current
strip-composited discs remain bootstrap evidence only.

The native `horizon_locked_camera` contract now supplies the camera-side math.
It stores a physical position, normalized world-up direction, yaw, and clamped
pitch. Each query reconstructs an orthonormal forward/right/up basis, so
incremental floating-point rotations cannot accumulate roll. Strafe follows
camera right, lift follows world up, dolly follows camera forward, and orbit
places the camera at a measured distance behind its facing direction. The
contract accepts generic measured targets and does not know what kind of entity
is being viewed. Public Sagan controls and the mesh demo still remain to be
wired after the GPU mesh path exists.

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
