# Shader toolchain build plan

Sagan Render will compile its reviewed HLSL sources ahead of time on one pinned
Linux x86-64 tool host. That host produces DXIL for D3D12, SPIR-V for Vulkan,
and MSL for Metal. Windows, Linux, and macOS renderer builds consume those
generated artifacts; they do not compile shaders or require DXC or
SDL_shadercross at runtime.

The isolated generator is implemented by
`scripts/build-shader-toolchain-linux.sh`. It changes no runtime shader path.

## Selected build graph

The exact inputs and CMake decisions are recorded in
`third_party/shader-toolchain-build.lock`:

- SDL 3.4.16 and SDL_shadercross use the commits already recorded by their
  repository locks.
- SPIRV-Cross uses its directly pinned SDL_shadercross gitlink. It is the only
  source gitlink selected for the generator build.
- DXC uses Microsoft's pinned Linux x86-64 `v1.9.2602` release asset and its
  recorded SHA-256 digest.
- SDL_shadercross is configured non-vendored and static, with the CLI enabled.
  Installation, runtime installation, tests, shared libraries, and leak-check
  support are disabled.

The generator therefore needs separately configured SDL and SPIRV-Cross
packages plus the pinned DXC binary package. It does not initialize or build
the SDL_shadercross SPIRV-Headers, SPIRV-Tools, or DirectXShaderCompiler source
gitlinks. Those commits remain recorded in `third_party/sdl-shadercross.lock`
so the upstream graph stays auditable, but they are outside this selected
build.

`SDLSHADERCROSS_CLI_STATIC=ON` makes the generator easier to isolate from its
build directory. It does not authorize publication of that executable. The
only planned cross-platform outputs are reviewed shader artifacts and their
provenance metadata.

## Why one generator host

Shader behavior should not change because a developer happens to build on a
different desktop. A single pinned tool environment gives DXIL, SPIR-V, and MSL
one source revision, one compiler revision, and one reproducible validation
point. The native renderer still tests every artifact on its actual backend:
D3D12 on Windows, Vulkan on Linux, and Metal on macOS.

This also keeps the large DXC source build and its nested dependency graph out
of ordinary renderer builds. macOS does not need a local DXC distribution just
to consume MSL generated and reviewed by the tool host.

## License and installation boundary

SDL and SDL_shadercross use the zlib license. SPIRV-Cross source files identify
their license as `Apache-2.0 OR MIT`. The exact pinned Linux DXC archive carries
`LICENSE-LLVM.txt` and `LICENSE-MS.txt`; its licensing must not be reduced to a
single shorthand label.

The initial generator treats these components as build tools and does not ship
their executables or libraries with Sagan Render. Build caches and provenance
must retain the upstream license and notice files. If a future package starts
redistributing a tool binary or runtime library, that is a new distribution
decision and requires a fresh license/install review before the lock changes.

## Running the generator

On an x86-64 Linux host with CMake, Ninja, Git, curl, a C/C++ compiler, and
SDL's Linux development dependencies:

```bash
bash scripts/build-shader-toolchain-linux.sh
```

The script reads the lock, cross-checks the existing SDL and SDL_shadercross
locks, verifies the DXC archive digest, initializes only the selected
SPIRV-Cross gitlink, and builds each dependency in an ignored directory under
`build/shader-toolchain/`. The verified DXC archive is extracted at
SDL_shadercross's fixed non-vendored lookup path inside that ignored source
checkout. The script rejects an incomplete DXC cache instead of silently
deleting or repairing it. After building the static CLI, it executes
`shadercross --help` with the pinned DXC library path and writes
`build/shader-toolchain/toolchain-report.txt`.

CI runs this complete build and smoke test on Ubuntu. The report explicitly
states that runtime artifacts were not replaced.

`scripts/generate-material-shaders-linux.sh` then compiles both stages of
`shaders/material/lit_mesh.hlsl` to DXIL, SPIR-V, MSL, and reflection JSON. It
records source, manifest, tool, and artifact hashes beside those outputs. CI
generates the complete set twice, validates each binary or textual format and
the declared resource counts, and requires the two directories to be
byte-identical. The reviewed output directory is retained as CI evidence.

These generated files still do not replace the bootstrap runtime fixtures.
The next boundary is to review the backend entry-point metadata, commit an
artifact-consumption policy, and load the project-owned shaders in an actual
depth-tested GPU pipeline.
