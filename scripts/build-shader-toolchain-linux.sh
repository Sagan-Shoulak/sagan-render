#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if [[ "$(uname -s)" != "Linux" || "$(uname -m)" != "x86_64" ]]; then
  echo "The pinned shader generator requires Linux x86_64." >&2
  exit 1
fi

for command in cmake curl git ninja tar; do
  if ! command -v "$command" >/dev/null 2>&1; then
    echo "Missing shader-generator prerequisite: $command" >&2
    exit 1
  fi
done

lock=third_party/shader-toolchain-build.lock
read_lock() {
  local key="$1"
  local value
  value="$(sed -n "s/^${key} = \"\(.*\)\"$/\1/p" "$lock")"
  if [[ -z "$value" ]]; then
    echo "Missing shader-toolchain lock value: $key" >&2
    exit 1
  fi
  printf '%s\n' "$value"
}

sdl_version="$(read_lock sdl_version)"
sdl_commit="$(read_lock sdl_source_commit)"
shadercross_commit="$(read_lock shadercross_source_commit)"
spirv_cross_commit="$(read_lock spirv_cross_source_commit)"
dxc_release="$(read_lock dxc_release)"
dxc_asset="$(read_lock dxc_linux_x64_asset)"
dxc_url="$(read_lock dxc_linux_x64_url)"
dxc_sha256="$(read_lock dxc_linux_x64_sha256)"

[[ "$sdl_commit" == "$(sed -n 's/^source_commit = "\(.*\)"$/\1/p' third_party/sdl3.lock)" ]]
[[ "$shadercross_commit" == "$(sed -n 's/^source_commit = "\(.*\)"$/\1/p' third_party/sdl-shadercross.lock)" ]]
[[ "$spirv_cross_commit" == "$(sed -n 's/^spirv_cross_commit = "\(.*\)"$/\1/p' third_party/sdl-shadercross.lock)" ]]

dependency_root="$repo_root/build/shader-toolchain/dependencies"
build_root="$repo_root/build/shader-toolchain/build"
install_root="$repo_root/build/shader-toolchain/install"
report="$repo_root/build/shader-toolchain/toolchain-report.txt"
mkdir -p "$dependency_root" "$build_root" "$install_root"

sdl_source="$(bash scripts/fetch-sdl3-source.sh)"
shadercross_source="$(bash scripts/fetch-shadercross-source.sh)"
[[ "$(git -C "$shadercross_source" rev-parse HEAD)" == "$shadercross_commit" ]]

echo "Initializing only the pinned SPIRV-Cross gitlink..."
git -C "$shadercross_source" submodule update --init --depth 1 external/SPIRV-Cross
spirv_cross_source="$shadercross_source/external/SPIRV-Cross"
[[ "$(git -C "$spirv_cross_source" rev-parse HEAD)" == "$spirv_cross_commit" ]]

dxc_archive="$dependency_root/$dxc_asset"
# SDL_shadercross intentionally fixes this non-vendored lookup path in its
# CMakeLists.txt, so place the verified binary package exactly there.
dxc_root="$shadercross_source/external/DirectXShaderCompiler-binaries"
if [[ ! -f "$dxc_archive" ]]; then
  curl --fail --location --output "$dxc_archive" "$dxc_url"
fi

if command -v sha256sum >/dev/null 2>&1; then
  actual_dxc_sha256="$(sha256sum "$dxc_archive" | cut -d ' ' -f1)"
else
  actual_dxc_sha256="$(shasum -a 256 "$dxc_archive" | cut -d ' ' -f1)"
fi
if [[ "$actual_dxc_sha256" != "$dxc_sha256" ]]; then
  echo "DXC checksum mismatch: expected $dxc_sha256, got $actual_dxc_sha256" >&2
  exit 1
fi

if [[ ! -f "$dxc_root/include/dxc/dxcapi.h" ]]; then
  if [[ -e "$dxc_root" ]]; then
    echo "Incomplete DXC cache; remove this exact directory and retry: $dxc_root" >&2
    exit 1
  fi
  mkdir "$dxc_root"
  tar -xzf "$dxc_archive" -C "$dxc_root"
fi
[[ -f "$dxc_root/include/dxc/dxcapi.h" ]]
[[ -f "$dxc_root/lib/libdxcompiler.so" ]]
[[ -f "$dxc_root/LICENSE-LLVM.txt" ]]
[[ -f "$dxc_root/LICENSE-MS.txt" ]]

sdl_build="$build_root/SDL3-$sdl_version"
sdl_install="$install_root/SDL3-$sdl_version"
cmake -S "$sdl_source" -B "$sdl_build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$sdl_install" \
  -DSDL_SHARED=OFF \
  -DSDL_STATIC=ON \
  -DSDL_TEST_LIBRARY=OFF \
  -DSDL_TESTS=OFF
cmake --build "$sdl_build" --parallel 2
cmake --install "$sdl_build"
[[ -f "$sdl_install/include/SDL3/SDL.h" ]]

spirv_cross_build="$build_root/SPIRV-Cross"
spirv_cross_install="$install_root/SPIRV-Cross"
cmake -S "$spirv_cross_source" -B "$spirv_cross_build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$spirv_cross_install" \
  -DSPIRV_CROSS_SHARED=OFF \
  -DSPIRV_CROSS_STATIC=ON \
  -DSPIRV_CROSS_CLI=OFF \
  -DSPIRV_CROSS_ENABLE_TESTS=OFF
cmake --build "$spirv_cross_build" --parallel 2
cmake --install "$spirv_cross_build"

shadercross_build="$build_root/SDL_shadercross"
cmake -S "$shadercross_source" -B "$shadercross_build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$sdl_install;$spirv_cross_install" \
  -DDirectXShaderCompiler_ROOT="$dxc_root" \
  -DSDLSHADERCROSS_VENDORED=OFF \
  -DSDLSHADERCROSS_DXC=ON \
  -DSDLSHADERCROSS_SHARED=OFF \
  -DSDLSHADERCROSS_STATIC=ON \
  -DSDLSHADERCROSS_SPIRVCROSS_SHARED=OFF \
  -DSDLSHADERCROSS_CLI=ON \
  -DSDLSHADERCROSS_CLI_STATIC=ON \
  -DSDLSHADERCROSS_CLI_LEAKCHECK=OFF \
  -DSDLSHADERCROSS_WERROR=ON \
  -DSDLSHADERCROSS_INSTALL=OFF \
  -DSDLSHADERCROSS_INSTALL_RUNTIME=OFF \
  -DSDLSHADERCROSS_TESTS=OFF
cmake --build "$shadercross_build" --target shadercross --parallel 2

shadercross_cli="$shadercross_build/shadercross"
[[ -x "$shadercross_cli" ]]
LD_LIBRARY_PATH="$dxc_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  "$shadercross_cli" --help >/dev/null

{
  echo "generator_host=linux-x86_64"
  echo "sdl_commit=$sdl_commit"
  echo "shadercross_commit=$shadercross_commit"
  echo "spirv_cross_commit=$spirv_cross_commit"
  echo "dxc_release=$dxc_release"
  echo "dxc_sha256=$actual_dxc_sha256"
  echo "generated_formats=dxil,spirv,msl"
  echo "cli=$shadercross_cli"
  echo "dxc_library_path=$dxc_root/lib"
  echo "runtime_artifacts_replaced=false"
} > "$report"

echo "Shader generator ready: $shadercross_cli"
echo "Toolchain report: $report"
