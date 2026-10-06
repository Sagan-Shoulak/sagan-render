#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

plan=third_party/shader-toolchain-build.lock
source_lock=third_party/sdl-shadercross.lock
sdl_lock=third_party/sdl3.lock
generator=scripts/build-shader-toolchain-linux.sh

grep -q '^generator_host = "linux-x86_64"$' "$plan"
grep -q '^distribution_model = "build-tool-only"$' "$plan"
grep -q '^dxc_release = "v[0-9][0-9.]*"$' "$plan"
grep -q '^dxc_linux_x64_url = "https://github.com/microsoft/DirectXShaderCompiler/releases/download/' "$plan"
grep -q '^dxc_linux_x64_sha256 = "[0-9a-f]\{64\}"$' "$plan"
grep -q '^dxc_license_notices = "LICENSE-LLVM.txt,LICENSE-MS.txt"$' "$plan"

for option in \
    SDLSHADERCROSS_VENDORED \
    SDLSHADERCROSS_DXC \
    SDLSHADERCROSS_SHARED \
    SDLSHADERCROSS_STATIC \
    SDLSHADERCROSS_SPIRVCROSS_SHARED \
    SDLSHADERCROSS_CLI \
    SDLSHADERCROSS_CLI_STATIC \
    SDLSHADERCROSS_CLI_LEAKCHECK \
    SDLSHADERCROSS_WERROR \
    SDLSHADERCROSS_INSTALL \
    SDLSHADERCROSS_INSTALL_RUNTIME \
    SDLSHADERCROSS_TESTS
do
    count="$(grep -c "^cmake_${option} = \"" "$plan")"
    [[ "$count" == 1 ]]
    value="$(grep "^cmake_${option} = " "$plan" | cut -d '"' -f2)"
    grep -q -- "-D${option}=${value}" "$generator"
done

grep -q '^cmake_SDLSHADERCROSS_VENDORED = "OFF"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_DXC = "ON"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_SHARED = "OFF"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_STATIC = "ON"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_SPIRVCROSS_SHARED = "OFF"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_CLI = "ON"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_CLI_STATIC = "ON"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_INSTALL = "OFF"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_INSTALL_RUNTIME = "OFF"$' "$plan"
grep -q '^cmake_SDLSHADERCROSS_TESTS = "OFF"$' "$plan"

grep -q '^selected_source_gitlink = "SPIRV-Cross"$' "$plan"
grep -q '^excluded_source_gitlinks = "SPIRV-Headers,SPIRV-Tools,DirectXShaderCompiler"$' "$plan"
grep -q '^generated_formats = "dxil,spirv,msl"$' "$plan"
grep -q '^published_tool_binary = false$' "$plan"
grep -q '^published_runtime_library = false$' "$plan"

bash -n "$generator"
grep -q 'uname -s.*Linux' "$generator"
grep -q 'uname -m.*x86_64' "$generator"
grep -q 'submodule update --init --depth 1 external/SPIRV-Cross' "$generator"
grep -q 'actual_dxc_sha256.*dxc_sha256' "$generator"
grep -q -- '-DSDLSHADERCROSS_VENDORED=OFF' "$generator"
grep -q -- '-DSDLSHADERCROSS_CLI_STATIC=ON' "$generator"
grep -q -- '-DSDLSHADERCROSS_INSTALL=OFF' "$generator"
grep -q 'runtime_artifacts_replaced=false' "$generator"

for excluded in external/SPIRV-Headers external/SPIRV-Tools external/DirectXShaderCompiler; do
    if grep -q "submodule update.*$excluded" "$generator"; then
        echo "Generator initializes excluded source gitlink: $excluded" >&2
        exit 1
    fi
done

plan_sdl="$(grep '^sdl_source_commit = ' "$plan" | cut -d '"' -f2)"
lock_sdl="$(grep '^source_commit = ' "$sdl_lock" | cut -d '"' -f2)"
[[ "$plan_sdl" == "$lock_sdl" ]]

plan_shadercross="$(grep '^shadercross_source_commit = ' "$plan" | cut -d '"' -f2)"
lock_shadercross="$(grep '^source_commit = ' "$source_lock" | cut -d '"' -f2)"
[[ "$plan_shadercross" == "$lock_shadercross" ]]

plan_spirv_cross="$(grep '^spirv_cross_source_commit = ' "$plan" | cut -d '"' -f2)"
lock_spirv_cross="$(grep '^spirv_cross_commit = ' "$source_lock" | cut -d '"' -f2)"
[[ "$plan_spirv_cross" == "$lock_spirv_cross" ]]

echo "Shader toolchain plan passed: one Linux generator, pinned tools, selected gitlink only, no shipped compiler runtime."
