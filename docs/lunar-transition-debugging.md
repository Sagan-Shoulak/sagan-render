---
title: Lunar transition clipping and LOD diagnosis
status: review-needed
publication_ready: false
verified_in: null
verified_on: null
verified_by: null
---

# Lunar transition clipping and LOD diagnosis

Issue #13, within the #11 scene/UI demonstration. Baseline: `dev` at
`3f6de6a`; diagnostic input focuses Moon at time zero and clicks its surface
marker at 4,000 ms. A fixed 50 ms presentation step makes snapshots, input,
camera updates, and captures share one clock. Times below are the first
captured samples, not claims about the exact timing of a user's real-time run.

## Independent causes

| First sampled time | Rendering/state cause | Consequence |
| --- | --- | --- |
| 5,900 ms | Center ray reaches the front surface at about 721 km, inside the unchanged 1,000 km near plane. The pipeline draws both face orientations. | Front ground is cut away; the far/interior sphere remains visible. |
| 6,750 ms | True lunar altitude crosses the hard 5 km mesh-replacement threshold, but its near plane remains 1,000 km. | The finite local patch replaces the sphere and is itself clipped away. |
| 6,800 ms | The six-metre base boulder also uses appearance 3. Its computed altitude crosses 5 km, selecting the full-size lunar patch around the boulder's position. | A second, misplaced cap fills the view. Disabling the old patch hid this cap but did not fix the real Moon. |
| 8,300 ms | The marker fade/UI detail flag abruptly changes planetary near/far planes from 1,000 km / 500 billion metres to 1 m / 4 million metres. | Actual ground suddenly reappears and intersects the bogus boulder cap. |
| Throughout | Structures use 50-degree vertical FOV, while the Moon uses 60 degrees, with different depth projection. | Surface/structure projection and depth ordering disagree. |
| 9,000 ms | Settled local orbit converts the tangent frame differently from the hard-coded descent orientation, and viewport width changes from 912 to 600 immediately. | A smaller angular snap and a large framing jump. |

The large flat blue field therefore has multiple causes, not one bad texture.
The original scene capture assertions checked that frames differed, not that
the correct surfaces survived. A sampled-material ABI or launcher rollback
would not correct these geometry/identity/projection errors.

## Corrected contract

- The rock uses neutral primitive appearance 4. Its radius remains six metres.
- The Moon is one closed, six-face cubed sphere at every distance, with cubic
  spacing and approximately 6.6 m cells around the anchor. No distance enables
  a second sphere, patch, plane, or shell. Radius scaling also applies to the
  local mesh, so a small textured primitive cannot accidentally become a
  full-sized terrain cap.
- Shared edges have two incident triangles. Lunar indices are 32-bit;
  spherical UVs unwrap the antimeridian and world +Z is north.
- Structures and lunar geometry share the same camera projection. Near-plane
  adaptation depends continuously on radial lunar surface distance, not UI state
  or an off-screen body's forward-depth crossing.
- Descent and settled view share one tangent-frame conversion. Sidebar space
  eases in and out over the transition.
- The close observation remains available. Its 2048 x 2048 crop at 0.8 m/pixel
  covers 1,638.4 metres, not tens of kilometres. Altitude weight is continuous
  between 1 and 20 km; spatial weight fades over the footprint. Only the anchor
  cube face receives it, so it cannot recur on the antipode.

The raw LROC observation averaged approximately 31/255, versus roughly 169/255
at the global anchor, and includes baked shadows. Direct albedo blending makes
its footprint dark even with correct geometry. Upload now uses bounded
fine-scale luminance residuals against an 81-pixel (64.8 m) box average,
centered on global-anchor luminance. No source-image bytes were changed. This
is a documented neutral demo-detail convention, not recovered physical albedo,
normal mapping, displacement, or a production terrain tile catalog. The sphere
remains spherical; foundation skirts seat structures on that geometry.

## Repeatable evidence

Run the normal focused scene test; it builds with the pinned compiler and then
invokes the continuity regression. In Windows Git Bash:

```bash
sagan_executable="$(bash scripts/resolve-sagan.sh)"
bash tests/integration/scene_demo_test.sh "$sagan_executable"
```

For an already freshly built demo:

```bash
bash tests/integration/lunar_transition_test.sh build/scene-sagan-demo-test/scene-sagan-demo-test.exe
```

The portable scene test invokes the same regression with its native executable.
The test saves 124 BMP frames at 50 ms intervals under
`build/lunar-transition-test/`, plus `trace.txt` and `local-horizon.bmp`. Traces
record draw identity, radius, albedo map, camera position, rebased center,
forward/up axes, near/far planes, FOV, viewport, box extents, detail weight, and
terrain index count. Header text distinguishes approach, detail, and base UI
phases; the source transition variables determine orientation/descent timing.

Assertions require one actual Moon and one primitive rock, invariant complete
lunar topology, identical draw projections, stable final camera orientation,
orbital Moon pixels, nonblank close ground, and sampled close texture detail.
Native checks also verify closed geometry, local surface height, north-up UVs,
continuous detail weight, removal of uniform/broad illumination, and retention
of fine detail. A low-elevation close orbit is captured separately.

Local validation is Windows D3D12/DXIL. Vulkan/SPIR-V and Metal/MSL require
their CI runs; do not infer their pixel success from the Windows result.
The normal `bash scripts/scene_demo.sh` launcher was also rebuilt and captured
using the deterministic clock and the test-only `SAGAN_RENDER_TEST_STOP_AFTER_MS`
guard, so launcher verification did not leave a test window running.
Issue #13's production tile catalog, physical terrain/displacement, and broader
base/UI completion work are not closed by this fix. Language/math compiler pin
and sampled shader ABI remain unchanged; Linux launcher work is untouched.

## Earth-zoom mesh-origin crash

The owner subsequently reported `SAG-RUN-0999` with native message
`Mesh box center must be in front of the camera` while zooming Earth. This is
not an Earth texture or UI error. The Moon draw reused a box transform for its
surface anchor, incorrectly requiring that coordinate origin to be in front of
the camera even after the actual Moon center had passed the queue's visibility
check.

A deterministic Right/Enter Earth focus, followed by wheel inputs at 3500 and
4000 ms, reproduced the exact error at 4435 ms with a 1 ms clock; 4434 ms was
the last completed frame. Reducing the second wheel input from 6 to 2 places
the same crossing at the 4450 ms sample on the standard 50 ms clock. The native
case has Moon center depth approximately +1,421,830 m and anchor depth
approximately -315,572 m. The Moon is off-screen during this Earth view, but its
coordinate origin is not an object visibility center.

`prepare_oriented_mesh_draw` now builds the same affine MVP and normal matrices
for finite coordinate origins on either side of the eye plane. GPU clipping
evaluates transformed vertices, not the mesh's origin. The box wrapper retains
the original center-in-front guard. No source texture, detail weighting, mesh,
foundation datum, north-up orientation, projection equation or dependency pin
changes are part of this fix.

After removing the crash, visual inspection exposed a second linked fault:
Earth disappeared at the crossing. The shared near plane fell from about
174,804 m at 4400 ms to 1 m at 4450 ms even though the Moon was over 382 million
metres away. With a 500-billion-metre far plane, Earth's float MVP depth row
rounded to its clip-W row; every Earth fragment had clear depth 1 and failed
the strict LESS test. The old global blue-pixel check counted HUD/guide colors
and therefore accepted the black viewport.

Near-plane adaptation now uses one percent of radial surface distance, clamped
to the same 1 m / 1000 km bounds. This stays continuous and conservative near
the lunar ground without treating an off-screen object's forward depth as
surface proximity. The Earth sequence now keeps near=1000 km and requires more
than 5000 blue Earth pixels in a central disk region excluding HUD/guide colors.
The local D3D12 capture has 25,127 such pixels at all three checkpoints.

`lunar_anchor_projection_test.cpp` checks signed rear/zero/front origins,
positive vertex clip W, finite matrices, unchanged front-origin transforms,
non-finite rejection and the strict box guard. `earth_zoom_test.sh` rejects
stale BMPs and requires new frames at 4400, 4450 and 4500 ms, including the
previously failing sample, with the existing scene/white/Earth pixel validation
and clean exit. It also reproduces the float clear-depth collapse, requires
visible central-disk Earth pixels and records the shared clipping planes.
Windows and portable scene entry points run it and retain
`build/earth-zoom-test/` evidence. The complete 124-frame lunar/base regression
continues to guard close detail, grounded structures and transition continuity.
