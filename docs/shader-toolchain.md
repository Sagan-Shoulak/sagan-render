# Shader toolchain build plan

Sagan Render will compile its reviewed HLSL sources ahead of time on one pinned
Linux x86-64 tool host. That host produces DXIL for D3D12, SPIR-V for Vulkan,
and MSL for Metal. Windows, Linux, and macOS renderer builds consume those
generated artifacts; they do not compile shaders or require DXC or
SDL_shadercross at runtime.

This is a build plan, not yet a functioning artifact generator. The current
chunk initializes no submodules, downloads no binaries, and changes no runtime
shader path.

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
their license as `Apache-2.0 OR MIT`. The pinned Microsoft DXC release carries
its own `LICENSE.TXT` and third-party notices; its license must not be reduced
to a single shorthand label.

The initial generator treats these components as build tools and does not ship
their executables or libraries with Sagan Render. Build caches and provenance
must retain the upstream license and notice files. If a future package starts
redistributing a tool binary or runtime library, that is a new distribution
decision and requires a fresh license/install review before the lock changes.

## Next implementation chunk

The next chunk may create the isolated Linux generator script. It must verify
all downloads, initialize only the selected SPIRV-Cross gitlink, configure the
locked CMake values exactly, compile the CLI, and record tool versions. It must
stop before replacing bootstrap shaders until generated artifacts and
reflection metadata have deterministic tests.
