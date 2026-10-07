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

When selected by the exact workspace lock, `scripts/workspace-build.sh` emits
the window demo's linked C++ and `scripts/workspace-test.sh` runs the same two
auto-closing checks. The workspace coordinator supplies its pinned compiler
and combined package index through `SAGAN_EXECUTABLE` and
`SAGAN_PACKAGE_INDEX`. Manual demo launchers instead resolve the repository's
exact compiler pin automatically when no executable is supplied.

Install or build compatible Sagan, MSYS2 UCRT64 g++, and a Windows desktop
session. From the candidate root in Git Bash on the current development
machine:

```bash
export SAGAN_EXECUTABLE=/c/Users/joeps/coding/sagan/bin/sagan.exe
bash tests/integration/window_contract_test.sh
bash tests/integration/ui_contract_test.sh
bash tests/integration/scene_contract_test.sh
bash tests/integration/scene_demo_test.sh
bash tests/integration/ui_gpu_demo_test.sh
bash tests/integration/window_bridge_test.sh
bash tests/integration/shape_text_test.sh
bash tests/integration/sdl_window_probe_test.sh
bash tests/integration/sdl_gpu_triangle_probe_test.sh
```

The window contract test has no display dependency and validates logical and
drawable sizes, display scale, focus, resize notification, close, and cleanup
state. The UI contract test is also display-independent. It validates logical
row/column layout, clipping, text shaping requests, overlay hit order, keyboard
focus, activation, pointer capture, and cleanup. It also proves that a fixed
physical orthographic span is independent of display scaling and writes an
inspectable `build/ui-contract/ui-layout.svg` artifact. The GPU UI demo test
consumes the same handle-free draw list, composes clipped rectangles and
deterministic bitmap glyph cells through SDL GPU, checks exact text, overlay,
and scene pixels. It saves a fixed 960×540 reference plus an injected 800×600
logical-target resize capture, proving that reallocation reflows the UI instead
of stretching the original frame without relying on a headless window manager
to honor a resize request. Its built-in font covers only the
demonstration text; it validates the shaping and drawing seam rather than
claiming production Unicode typography. The scene contract test is also
display-independent. It compiles the public Sagan `render.scene` adapter
surface, then verifies immutable snapshots, stable IDs, unit-preserving origin
subtraction, camera basis and projection matrices, sphere/frustum visibility,
label anchors, and camera reframing near coordinates of `1e15 meter`. It writes
`build/scene-contract/scene-camera.svg`; because it has no window or backend
dependency, the same test runs on Windows, Linux, and macOS. The window test
opens and closes its own window after roughly 100 ms. The shape/text
test closes after roughly 150 ms and checks that its own generated BMP is
960×540. The SDL probe opens a 960×540 high-DPI, resizable SDL3 window,
paints a recognizable test frame, presents it, saves the surface to BMP, and
checks clean shutdown. It downloads the official SDL 3.4.16 MinGW archive only
when absent and rejects a checksum mismatch. The window and shape/text checks passed with the extracted package, current Sagan 4.9.5
development executable, and no monorepo launcher resource object. On their
first run without `SAGAN_EXECUTABLE`, manual demos clone the Sagan source
revision in `sagan-source-commit.txt` and build it under the ignored
`build/sagan-pinned-<commit>/` cache. This requires network access, Git, Make,
and the compiler prerequisites for Sagan; later runs work from that cache.
The resolver intentionally ignores an arbitrary `sagan` on PATH because its
private bridge vocabulary may not match this repository. Set
`SAGAN_EXECUTABLE` only to make an explicit reviewed override. Set
`SAGAN_PACKAGE_INDEX` only for a reviewed alternate catalog; otherwise the
candidate-local index is used. Do not kill another user-controlled demo window
while testing.

The corresponding manual demos are:

```bash
bash scripts/window_demo.sh
bash scripts/shape_text_demo.sh
bash scripts/ui_gpu_demo.sh
bash scripts/loading_demo.sh
bash scripts/toolbar_demo.sh
bash scripts/scene_demo.sh
```

These open native windows. The tests set `SAGAN_RENDER_AUTOCLOSE_MS` to avoid
leaving them open. The bridge uses an embedded custom icon if available and
a Windows stock icon otherwise. The latter is a compatibility fallback, not
a decision about final product branding. The current non-Windows backend
throws an explicit unsupported-platform error; do not claim Linux/macOS
rendering based on a passing Windows test.

## Cross-platform SDL foundation probe

The `portable-sdl-window-probe` CI matrix builds the pinned SDL 3.4.16 source
archive and runs the same source probe on Linux and macOS. Linux uses Xvfb;
macOS uses the runner's Cocoa session. Each job must report window creation and
cleanup and upload its rendered BMP. These are native-window dependency checks,
not proof that the Sagan-facing renderer has been ported and not a GPU pipeline
test.

On a Linux or macOS development machine with CMake, Ninja, a C++23 compiler,
and the corresponding SDL platform build dependencies:

```bash
bash tests/integration/sdl_window_probe_portable_test.sh
```

On headless Linux, run the script through `xvfb-run --auto-servernum`. The
source archive and its digest are recorded in `third_party/sdl3.lock`; never
silently substitute a system SDL version. The probe's software-surface drawing
exists only to isolate window behavior from the separate GPU validation below.

## Cross-platform SDL GPU foundation

The GPU probe creates a real backend device, renders a bufferless RGB triangle
into a deterministic 256×256 offscreen texture, blits that texture to the
window swapchain, downloads it through a transfer buffer, and validates the
returned pixels before retaining a BMP. Windows explicitly requires D3D12 and
DXIL; Linux requires Vulkan and SPIR-V; macOS requires Metal and MSL.

On Windows in Git Bash:

```bash
bash tests/integration/sdl_gpu_triangle_probe_test.sh
```

On Linux or macOS after installing the platform dependencies described above:

```bash
bash tests/integration/sdl_gpu_triangle_probe_portable_test.sh
```

Headless Linux must run the portable test through `xvfb-run --auto-servernum`
and have a Vulkan implementation such as Mesa lavapipe. Both scripts download
only the pinned bootstrap shader fixtures and reject checksum mismatches. The
fixtures validate shader loading; issue #16 replaces them with the project's
own ahead-of-time shader toolchain.

The generated capture and report live under ignored `build/sdl-gpu/`. A passing
test requires device, command-buffer, render-pass, swapchain, triangle, fence,
readback, and cleanup evidence. If the SDL foundation must be rolled back,
retain the current Win32 bridge and revert the SDL probe/shader changes as one
unit; current package-facing canvas behavior does not depend on them.

## Scene and camera demo

The scene/camera precision demo uses the same GPU host with generic static
render samples positioned near `1e15 meter`. Run it on Windows with
`bash scripts/scene_demo.sh`, or on Linux/macOS with
`bash scripts/scene_demo_portable.sh`. Right-button drag orbits the camera around
its current target, the wheel zooms by changing measured target distance, Left
and Right select samples, Enter smoothly focuses the selected sample, and
pointer clicks pick projected samples. The demo draws presentation-only Earth
and Moon orbit guides from unit-typed sampled paths; no Sun orbit or secondary
inset camera is drawn. Focus and wheel zoom expose the lunar scale in the main
viewport.
Focused CI captures initial, selected,
right-dragged, and wheel-zoomed frames and requires the relevant frames to
differ on D3D12, Vulkan, and Metal. The example contains no orbital dynamics;
its only motion is application-owned camera input.

The scene names the three fixture samples Sun, Earth, and Moon, uses their
approximate mean radii and separations, and draws a shared 1,024-triangle
smooth-normal UV sphere through the GPU compositor. It proves the indexed mesh,
depth, material, and camera seams, not production model loading, textures, or
multi-light fidelity. The Moon callout offset is presentation-only at system
scale; its stored position remains the measured fixture coordinate.

## Interactive UI GPU demo

On Windows in Git Bash:

```bash
bash scripts/ui_gpu_demo.sh
```

On Linux or macOS with the dependencies required by the portable SDL probe:

```bash
bash scripts/ui_gpu_demo_portable.sh
```

The demo compiles `examples/ui_sagan_demo/src/main.sagan`, starts at 960×540,
and presents through D3D12, Vulkan, or Metal.
Use Tab and Shift+Tab to move focus, Enter or Space to activate, P to toggle the
pause overlay, the mouse to activate buttons, and Escape to close. Resizing
recomputes logical layout and reallocates the offscreen target, so text retains
its logical size and aspect. The main view is exactly 200,000,000 km wide and
includes a derived 50,000,000 km scale bar; an Earth–Moon inset is exactly
1,000,000 km wide. Interactive runs start with both views exposed. CI launches
the same Sagan program at 960×540 and 800×600 for comparable layout and
physical-span evidence.

Sagan owns the composition, focus and activation state, hit-testing, and
unit-typed physical-to-logical projections. The C++ bridge retains native
window, event, bootstrap text-rasterization, and SDL GPU resource ownership.
The compiler commit in `sagan-source-commit.txt` must support the exact private
bridge signatures used by `render.ui`; changing either side requires updating
and validating the pin together.

## Loading-screen example

On Windows in Git Bash:

```bash
SAGAN_EXECUTABLE=/path/to/pinned/sagan.exe bash scripts/loading_demo.sh
```

On Linux or macOS:

```bash
SAGAN_EXECUTABLE=/path/to/pinned/sagan bash scripts/loading_demo_portable.sh
```

The example is authored in `examples/loading_sagan_demo/src/main.sagan`.
Press F to display the asset-failure presentation, R to restart the synthetic
loading cycle, and Escape to close. The synthetic driver is deliberately not a
game asset policy: applications replace it with their own progress, ready, and
failure signals while retaining the same presentation states.

Focused CI runs loading at 960×540, ready at 800×600, transition at 1024×576,
and failure at 960×540.
It freezes elapsed time and injects failure input through private environment
test hooks, checks captured BMP dimensions and content thresholds, and requires
cleanup reports on D3D12, Vulkan, and Metal. Captures and the report are under
ignored `build/loading-sagan-demo/`.

## Toolbar and controls example

On Windows in Git Bash:

```bash
SAGAN_EXECUTABLE=/path/to/pinned/sagan.exe bash scripts/toolbar_demo.sh
```

On Linux or macOS:

```bash
SAGAN_EXECUTABLE=/path/to/pinned/sagan bash scripts/toolbar_demo_portable.sh
```

The reusable behavior is in `libraries/render/src/controls.sagan`; the example
composition is in `examples/toolbar_sagan_demo/src/main.sagan`. Tab, Shift+Tab,
and arrow keys traverse enabled controls; Enter and Space activate; pointer
motion, press, capture, and release use the same action path. The middle control
is intentionally disabled. Narrow windows reflow the toolbar vertically.

Focused CI compares captures rather than relying only on process success.
Keyboard and pointer activation must be byte-identical; disabled activation
must match idle; forward and reverse traversal must agree; hover and press must
be visibly distinct; and horizontal/vertical runs must report cleanup. Evidence
lives under ignored `build/toolbar-sagan-demo/`.

## Windows dev-channel package

The minimum prerelease distribution is a source package for the compiler's
existing native-package loader. Produce it only from a clean reviewed `dev`
commit:

```bash
bash scripts/package-dev.sh
```

The command creates
`build/dev-package/sagan-render-0.5.1-dev.1-windows-source.zip` and its
`.sha256` file. The ZIP contains `LICENSE.txt`, `NOTICE.txt`,
`libraries/index.tsv`, the package manifest
and Sagan modules under `libraries/render/`, the private native bridge under
`libraries/render/native/`, and `PROVENANCE.txt`. Set `SAGAN_PACKAGE_INDEX` to
the extracted `libraries/index.tsv`. The catalog declares compiler
compatibility `^4.0.0`; the package is GPL-3.0-only and preserves Schematic
lineage; Windows is the only supported native backend.

For a clean-location consumption test, point `SAGAN_EXECUTABLE` at an installed
Sagan 4.9.5 executable and run:

```bash
export SAGAN_EXECUTABLE=/path/to/sagan-4.9.5/bin/sagan.exe
bash tests/integration/dev_package_test.sh
```

The test verifies the checksum, extracts away from the repository, copies only
consumer manifests and sources into that clean location, and exercises both
auto-closing windows through `sagan --run-package`. A missing native bridge or
incompatible compiler is a hard failure. Recover by removing the extracted
package, clearing `SAGAN_PACKAGE_INDEX`, and restoring the previously reviewed
package/index pair. To withdraw a published dev artifact, delete the GitHub
prerelease and its dev tag; do not replace an existing asset under the same
tag or checksum.

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
