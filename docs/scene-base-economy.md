---
title: Scene demo construction, power and life-support fixture
status: review-needed
publication_ready: false
verified_in: null
verified_on: null
verified_by: null
---

# Scene demo economy fixture

This owner-requested issue #11 fixture lives in
`examples/scene_sagan_demo/src/base_economy.sagan`, operated through the
[Slots UI](scene-base-ui.md). It is session-only application state, not a renderer
API, authoritative Space Game policy or engineering-sized crew/base. Resource
and power accounting are functional, but yields, consumption rates and constant
illumination are fictional gameplay settings. There is no persistence, new
machine geometry, deposit depletion, solar ephemeris, mortality or thermal model.
Lunar geometry, camera, native bridges, material ABI and pins are unchanged.

## Bulk-only construction

All costs below are kilograms. Motors, electronics, screens, heads and stacks
are removed from inventory and recipes; manufacturing is abstracted into the build.

| Resource | Sifter | Ice drill | Electrolysis | Kit fabricator |
| --- | ---: | ---: | ---: | ---: |
| Steel | 150 | 400 | 180 | 250 |
| Aluminum | 30 | 50 | 80 | 40 |
| Copper | 10 | 25 | 60 | 20 |
| Polymers | 10 | 15 | 30 | 15 |
| Silicon | 5 | 5 | 10 | 5 |
| Powered build time | 10 s | 15 s | 20 s | 15 s |

Confirmation atomically spends the whole recipe once and reserves the slot.
Unaffordable jobs are disabled/rejected; occupied slots cannot be overwritten.
Canceling the chooser costs nothing. Each job draws 6 kW and pauses without
allocated power. It produces nothing and cannot receive repairs before completion.
Fractional starts never finish early; the one-second scheduler may finish next tick.

## Storage and bootstrap

Storage holds 10,000 kg. Initial stock totals 3,529 kg, including enough to
build one of every machine:

| Resource | Initial stock |
| --- | ---: |
| Steel | 1,400 kg |
| Aluminum | 300 kg |
| Copper | 125 kg |
| Polymers | 120 kg |
| Silicon | 100 kg |
| Water | 600 kg |
| Ice | 100 kg |
| Oxygen | 120 kg |
| Hydrogen | 0 kg |
| Food | 600 kg |
| Repair kits | 8 items |

Bulk quantities are integer grams; traces display three decimals. Kits are
8 kg items. Additions obey mass capacity. Transactions check all inputs first;
building, feeding, repair and gas venting are explicit inventory sinks.

## Gameplay-first regolith sifting

The sifter processes a notional 100 kg soil every **60 seconds**, recovering
materials rather than a resource called "sifted regolith":

| Resource | kg per batch | Fictional recovered fraction |
| --- | ---: | ---: |
| Oxygen | 25 | 25% |
| Silicon | 20 | 20% |
| Aluminum | 12 | 12% |
| Steel / iron-derived alloy | 6 | 6% |
| Ice | 1 | 1% |
| Water | 0.2 | 0.2% |
| Polymers / volatile feedstock | 0.1 | 0.1% |
| Copper | 0.04 | 0.04% |
| Hydrogen | 0.01 | 0.01% |

The remaining 35.65 kg is untracked residue. These are not a Shackleton assay,
physical extraction efficiency or realistic throughput.
[NASA's composition overview](https://science.nasa.gov/moon/composition/) provides
major-element context; [lunar volatile research](https://science.nasa.gov/lunar-science/focus-areas/volatile-cycles/)
provides polar context. Copper/carbon feedstock are gameplay-boosted.
Reduction, alloying, oxygen release and polymer synthesis are bundled into the
fixture, not claimed to happen through screening alone.
[NASA's icy-regolith project](https://techport.nasa.gov/projects/154566) discusses
volatile extraction and possible polymer manufacturing, not these recipes.
There is no component assembly, unique-item output or food production.

Independent restock targets prevent abundant outputs choking maintenance inputs:
steel 1,500 kg, aluminum 600 kg, copper 200 kg, polymers 120 kg, silicon 500 kg,
water 300 kg, ice 1,500 kg, oxygen 500 kg, hydrogen 50 kg and kits 20 items.
Seed stock above a target is retained. Targets regulate production, not storage.

## Ice-only drill and shared water/ice feed

**The drill produces only ice:** 9.5 kg every five powered seconds, never steel.

Water and ice remain separate visible stocks, with a shared H2O total. Life
support and electrolysis use one feed-line helper: consume liquid first, then
thaw only the missing ice gram for gram. There is no transfer queue, hauling
operation or extra machine slot. Each consumer adds 2 kW in-line thawing demand
when needed. Heaters/pumps are integrated into consumer/storage plumbing;
temperature, phase-change physics and heat transfer remain abstracted.

Electrolysis consumes 9 kg H2O every five powered seconds, producing 8 kg oxygen
and 1 kg hydrogen while retaining 30 kg shared H2O for life support.
The simplified 9:8:1 split conserves mass; see
[DOE's electrolysis overview](https://www.energy.gov/cmei/fuels/hydrogen-production-electrolysis).
An oxygen-full plant idles. Excess hydrogen above its target is explicitly
vented/tracked, so a full unused hydrogen buffer cannot deadlock oxygen supply.

## Functional power and battery

The continuously illuminated fixture generates 120 kW with a healthy solar
system. Every one-second tick prioritizes essential/life-support loads, then
industrial slots in slot order. Unserved machines and construction pause.
Stock-satisfied/input-starved machines idle rather than draw their working load.

| Load | kW |
| --- | ---: |
| Chassis, solar controller, battery conditioning and storage | 6 |
| Life support | 18 |
| Sifter | 25 |
| Ice drill | 20 |
| Electrolysis | 35 |
| Kit fabricator | 10 |
| Life-support thawing when needed | 2 |
| Electrolysis thawing when needed | 2 |
| Each construction job, instead of finished machine load | 6 |

All four working machines plus both thawing loads total **118 kW**, leaving
**2 kW surplus**. The 200 kWh battery starts at 160 kWh; surplus charges and
deficits discharge it, with 90% efficiency each way and a 120 kW limit.
Integer watt-second energy stays in [0, capacity]; full batteries curtail surplus.
A diagnostic illumination input proves depletion, load shedding and recovery.
Resources shows actual generation, demand, served load, net power and stored
energy, not hard-coded presentation values.

## Life support and sustainable balance

Every five powered seconds, life support consumes 0.3 kg H2O, 7.6 kg oxygen and
0.025 kg food as an atomic feeding cycle. These accelerated rates are demo
balance settings, not biological consumption or an engineering specification.
Shortages show LIFE SUPPLY LOW; mortality is not implemented.

| Loop, before buffers fill | Supply per minute | Demand per minute | Margin |
| --- | ---: | ---: | ---: |
| Full-load power | 120 kW | 118 kW | 2 kW |
| Electrolysis oxygen alone | 96 kg | 91.2 kg life support | 4.8 kg |
| Drill + sifter H2O | 115.2 kg | 108 kg electrolysis + 3.6 kg life support | 3.6 kg |
| Sifter copper during full-speed kit refill | 0.04 kg | 0.03 kg fabricator | 0.01 kg |
| Sifter copper during average maintenance | 0.04 kg | 0.0036 kg maintenance | 0.0364 kg |

Life support uses 95% of electrolysis output. Sifter oxygen is an extra regulated
byproduct, not required to make that margin positive. Buffers eventually fill
and production idles; abundant building materials use restock targets rather
than artificially identical surpluses.

**Food is the intentional exception:** no machine makes it. The 600 kg reserve
lasts roughly 33 hours 20 minutes of uninterrupted fixture feeding, then requires
resupply. Mineral repair and replenishable consumables are self-maintaining,
but this is not a permanently food-self-sufficient base.

## Exact automatic repairs from sifter minerals

The chassis, four mandatory systems and completed machines have health 0–100,
wearing one point every 60 seconds. The chassis starts at 85, mandatory systems
90, and completed machines 100.

Each kit restores **exactly 25 points**. At **75 health or below**, automatic
maintenance consumes kits until the system is above 75, checking every second
and prioritizing chassis/mandatory systems. Health 76–100, empty slots and
constructing machines cannot waste a kit. Manual repair follows the same rule.
Stored kits can restore offline services; zero health may need multiple kits.

The fabricator makes one 8 kg kit from **5 kg steel + 2 kg aluminum + 0.01 kg copper
+ 0.99 kg silicon**, taking 20 powered seconds when supplies/stock target permit.
All four inputs come directly from sifting. No ice, water, drill, electrolysis,
food, special component or imported material is needed to replenish a kit.

Nine systems average 0.36 kits/minute. Copper now supports 4 kits/minute, above
the fabricator's 3 kits/minute maximum. The previous 0.1 kg copper recipe used
0.30 kg/min while refilling the 20-kit buffer, causing the reported -0.26 kg/min
net despite positive long-term maintenance balance. The revised recipe leaves
+0.01 kg/min even during full-speed refill, without increasing lunar trace
abundance, changing kit mass, build costs or repair strength. Other mineral
buffers can still draw down during refill (steel supports 1.2 kits/minute);
they comfortably cover the 0.36 kits/minute maintenance average. All
simulation/maintenance continues in other tabs and orbital view. Resources and
Details remain read-only.

System summaries expose mixed balances instead of assigning one surplus label
to an entire machine. A draining reserve shows estimated time-to-empty at the
observed operating rate; zero/positive net has no countdown. Shared water/ice
is assessed as one H2O supply, and battery reserve uses efficiency-adjusted
stored-side discharge. These conditional estimates are presentation diagnostics,
not changes to production, recipes, repair scheduling or food consumption.

## Verification boundary

Tests execute the actual emitted Sagan classes: all bulk recipe shortages,
atomic costs, gram recovery, ice-only drilling, mineral-only kits, mass capacity,
fractional/power-paused construction, shared water-first/ice fallback, feeding
shortages, electrolysis conservation/gas-full behavior, finite food, 75/76 repair
boundaries, nine-system automatic repairs, battery charge/discharge/depletion,
essential power priority and frame-partition equality.
Full-speed kit refill additionally checks 0.04 kg/min copper production,
0.03 kg/min consumption and +0.01 kg/min net against actual stock deltas.

A two-hour unattended run removes all starter stock **except food**, performs no
manual repairs/imports, and checks positive power/charging, life-support supply
after cold-start bootstrap, bounded storage and durability. A separate
sifter/fabricator-only run proves repairs do not rely on drilling/electrolysis.
Live pointer/keyboard and 800x400 captures check recipes, disabled actions,
no wasted kits, construction, power/battery, food and kit counts.
Windows D3D12 evidence does not establish new Vulkan/Metal results.

The earlier dense 124-frame lunar continuity regression passed. Default
real-time Earth/Sun ancestor focus previously raised
`Mesh box center must be in front of the camera`. Unchanged dependency
`b2271bf` builds its lunar anchor transform through a box helper; an
isolated probe reproduces the guard with a front-facing Moon center and
rear-facing anchor. That is still a candidate explanation, not an exact failing
live-frame diagnosis or claimed fix. Lunar code remains untouched; evidence is
under `build/scene-base-ui-test/`.
