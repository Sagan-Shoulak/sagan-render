---
title: Scene demo base slots and panels
status: review-needed
publication_ready: false
verified_in: null
verified_on: null
verified_by: null
---

# Scene demo base UI

This issue #11 slice changes the combined orbital/base demo's sidebar, not its
lunar mesh, surface material, projection, camera path or fixture geometry.
The UI is Sagan-authored in `examples/scene_sagan_demo/src/base_ui.sagan`;
the session-only economy fixture lives alongside it in `base_economy.sagan`.
The native host only supplies existing input edges, logical dimensions and GPU
composition. No compiler, shader, SDL or dependency pin changes are required.

## Chassis and slots

The plain chassis is an inspectable base system. It defines four mandatory,
type-restricted slots, initially filled with a solar power system, battery
system, life support system and storage system. Four separate machine slots
start empty. Chassis capacity and mandatory type lookup are fixture data,
separate from control drawing and input. The plain form has no bonuses.

Slots is the initial tab and the only customization panel. Clicking an empty
machine slot opens a chooser with a regolith sifter, mining drill, electrolysis
plant and repair-kit fabricator. Choose a type, inspect its cost/stock and build
time, then Start Build; Cancel or Escape changes neither inventory nor the slot.
Insufficient stock disables Start for pointer and keyboard input. Confirmation
rechecks the complete recipe, deducts it once and starts a visible countdown.
Occupied/constructing slots open an inspector and cannot be overwritten. Mandatory
systems and the chassis can be inspected but cannot be replaced with a machine.

Machines cannot produce or be repaired before construction completes. The
chassis, mandatory systems and completed machines have durability. Kits restore
exactly 25 points and are automatically consumed at 75 health or below; manual
Repair follows the same rule and cannot waste a kit on 76-99 health. Construction, production and
wear continue in other tabs and orbital view. See [the economy fixture](scene-base-economy.md)
for exact recipes, initial inventory, fictional regolith recovery and the closed
repair loop. Closing the process resets the session. Machine models, upgrades,
demolition, chassis replacement and persistence remain unimplemented. Existing
world props remain visualization fixtures, not new buildings created by slots.
These owner-requested demo rules are not renderer APIs or authoritative Space
Game state.

## Live system context

Every chassis, mandatory-system and occupied-machine card shows its own operating
state and health, followed by resource-level balances. Running is separate from
surplus/deficit: an oxygen plant can be running while startup consumption still
draws down oxygen stock. A mixed badge counts gaining and draining resources
rather than hiding a mineral deficit behind an output surplus. Empty buffers are
counted separately. The card highlights key outputs and its shortest-lived
draining input; the inspector shows all relevant resources.

Green `SURPLUS`, red `DEFICIT`/`EMPTY` and neutral `BALANCED`/`SAMPLING` labels use
text as well as color. Each resource row shows the signed base-wide operating
net, its current reserve and an estimated time until empty when stock is draining.
Zero or positive flow shows `NO DRAIN`, not a fabricated countdown; empty stock
says `EMPTY NOW` or `EMPTY - REFILLING`. A startup resource without a flow sample
waits for sampling. Bulk rates use `kg/min`, kits use `kits/min`, stored mass uses
`kg`, and grid/battery rates use `kW`. Slash, plus and percent glyphs are rendered
by the host's demonstration bitmap font rather than silently omitted.
Gold still highlights operating faults. Target-met idle, mineral shortage, H2O
reserve, construction/power pause, charging/discharging and full storage remain
distinct states.

Time-to-empty rounds upward from current stock divided by measured drain. Kit
estimates account for fractional item rates; battery estimates use the actual
stored-side discharge after efficiency, not just the grid shortfall. Estimates
assume the displayed rate holds, not that future targets, repair bursts, load
shedding or production changes are known. Storage and system summaries pool
water/ice as `H2O`, so consuming liquid while replenishing ice does not report a
false shared-supply deficit. An unused empty buffer cannot hide a genuinely
draining resource's countdown. Resources still lists separate water/ice stocks.

Inspectors show system-specific stored supplies, observed input/output rates,
rated capacity, power, durability and repair-kit consumption. Solar shows actual
output/grid balance; battery shows stored energy and the actual stored-side
charge/discharge rate after efficiency and capacity limits. Life support shows
H2O, oxygen and food use, shared H2O/oxygen balance and rated food reserve time.
Food remains flagged as a deficit because it has no local producer. Storage
shows every stock and its operating net; machines show their own inputs and
outputs, targets/reserves, mineral kit recipe or hydrogen venting as appropriate.
Chassis context covers installed slots, services, grid, supplies and maintenance.

Material telemetry counts actual successful operating transactions in completed
60-second windows; before the first minute it averages elapsed startup seconds.
The cards and inspector label this sampling window and distinguish rated throughput
from observed production. It is not an instantaneous or rolling average: after
the first minute, rates refresh once per minute, while state, stock and power
remain live. Imports and one-time construction costs are excluded from operating
rates; repair consumption is included. Bulk rates retain grams; kit rates use
fractional items, never kilograms. Electrolysis output includes vented hydrogen,
but venting is removed from stored-supply net, so it cannot appear as a surplus
in storage. These diagnostics do not change the economy's rules.

## Read-only panels

Resources shows bulk materials, finite food and repair kits, mass-based storage
usage, actual solar/load/net/served power and simulated battery charge. One-off
motors, electronics, screens, heads and stacks are removed. Water and ice share
a water-first, automatic-thaw supply line without another slot. Life support
consumes H2O, oxygen and food; supply/power shortages are visible. Building, production and repair
change the same inventory shown here; gram-scale traces are retained and visible.
Details shows the base name, operational status, body, marker coordinates,
marker altitude, chassis and bonuses. The marker's latitude is 30 degrees north
and longitude zero: it is a deterministic fixture, not real Shackleton geography.
The base name and demo-session status remain visible above all tabs.

## Input and sizing

Pointer activation requires press and release inside the same control. Resizing
or scrolling cancels captured presses. Tab/Shift-Tab traverse controls; Enter or
Space activates the focused control. The chooser traps focus, skips a disabled
Start button and blocks scene
picking, camera input and Return to Orbit. Escape cancels a chooser or inspector;
outside them, Escape or R returns from local view. Escape closes orbital view.
Other demos retain their previous Escape behavior.

At short window heights, wheel input over the sidebar scrolls its content and
does not zoom the camera. Painting and hit testing share the same visible bounds;
keyboard traversal brings focused slot cards into view. The demo opens at
1280x900 with a 520-unit sidebar and two card columns. At 960 units wide the
sidebar is 420 units; smaller windows use 312 units and one column. Painting,
pointer controls and focus navigation share the same responsive card geometry.
The responsive width is reserved through the existing eased descent, not a new
handoff or camera snap. Shared lunar depth/projection, meshes and materials are
unchanged.

## Verification

`scene_demo_test.sh` and `scene_demo_portable_test.sh` call
`scene_base_ui_test.sh` using their freshly built executable and emitted C++.
The state test exercises the actual emitted Sagan class, not a duplicate native
model. It checks chassis restrictions, four empty slots, confirmation and
cancellation, occupied protection, read-only panel behavior, pointer capture and
relayout. Deterministic pointer and keyboard scripts exercise the live demo,
including an 800x400 logical resize and return to orbit. Pixel assertions verify
empty/filled cards, cancellation, modal capture, recipes, shortage rejection,
countdown completion, no wasted repairs, power/battery, food and live kit production.
Balance assertions check mixed surpluses/deficits, empty stock, no-drain states,
shared H2O, rounded depletion times and battery efficiency. Label-width checks
cover both the rate and reserve lines. Pixel assertions require green copper,
red steel/food and visible depletion estimates, plus independent 5x7 patterns
for plus and slash glyphs, including `COPPER SURPLUS +0.01 kg/min` during refill.
The context sequence additionally opens and scrolls all nine live inspectors,
including a short-window life-support view. Actual emitted-state assertions check
per-system flows against inventory deltas, startup oxygen deficit versus steady
surplus, battery direction/efficiency, input/target/repair states, hydrogen vent
accounting, read-only inspection and text widths against the fixed glyph advance.
The emitted Sagan economy also tests bulk-only recipes, ice-only drilling,
sifted-only kit inputs, shared water/ice, finite food, real power allocation and
battery energy, automatic exact repairs, frame-partition independence and a
two-hour unattended maintenance run with all starter resources except food removed.
Evidence is retained under `build/scene-base-ui-test/`.

The private `SAGAN_RENDER_TEST_INPUT_FILE` hook feeds timed key, pointer, wheel and
logical-resize events through existing input state. It does not mutate base UI
state directly. `SAGAN_RENDER_TEST_CAPTURE_TIMES_MS` optionally limits sequence
captures to exact comma-separated timestamps; unset retains dense lunar capture
behavior. Canonical orbital/lunar tests explicitly resize to 960x540 through
`scene_test_size.txt`, keeping marker coordinates independent of the larger
manual window. UI tests retain targeted checkpoints including an 11-minute automatic
maintenance capture.

The canonical scene harness also uses the existing 50-millisecond frame-stepped
clock: time advances only after presentation. A slow software-rendered CI frame
therefore cannot jump past a capture's short automatic-close window. This fixes
the observed Windows CI missing-focus-capture failure without relaxing capture,
frame-difference or pixel assertions, or changing interactive rendering.

Normal runs leave these hooks unset. Launch the demo as before:

```bash
bash scripts/scene_demo.sh
```

Windows D3D12 evidence does not independently establish Vulkan/Metal support;
the portable CI jobs run the same tests. Issue #11's broader UI roadmap remains
open. PR #68 supplies the independently verified lunar continuity dependency;
the owner-authorized dev integration preserves that fix and leaves issue #11
open. This UI slice does not change the lunar renderer.

Local context verification uses the exact Sagan pin
`d8f6d50672908b852bb9003b8200a7b0875ac3f3`, MSYS2 g++ 15.2.0 and SDL 3.4.16.
Focused emitted-state/economy tests, D3D12 UI captures, UI/scene contracts,
toolbar interaction and the 124-frame lunar continuity/low-horizon regression
passed on 2026-10-11. Compiler/package, SDL and shader pins and renderer ABI are
unchanged. The slice affects the example and its existing focused CI entry
points, not production simulation ownership. No full-suite, merge, promotion
or release verification is claimed.
