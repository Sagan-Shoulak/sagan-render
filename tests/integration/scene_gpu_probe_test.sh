#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/scene-3d build/tmp
sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" tests/native/scene_gpu_probe.cpp \
  -L"$sdl_root/lib" -lSDL3 -o build/scene-3d/scene-gpu-probe.exe
cp "$sdl_root/bin/SDL3.dll" build/scene-3d/SDL3.dll
export SAGAN_RENDER_MATERIAL_SHADER_DIR=shaders/generated/material
export SAGAN_RENDER_SCENE_3D_CAPTURE_BMP=build/scene-3d/scene-3d.bmp
build/scene-3d/scene-gpu-probe.exe | tee build/scene-3d/scene-3d-report.txt
grep -q "SCENE_3D driver=direct3d12 indexed=1 depth=1 views=4 bodies=3" \
  build/scene-3d/scene-3d-report.txt
[[ "$(od -An -td4 -j18 -N8 build/scene-3d/scene-3d.bmp | tr -s ' ' | sed 's/^ //;s/ $//')" == "960 540" ]]
echo "3D scene GPU probe passed on D3D12: fixed-radius Sun, Earth, and Moon models are distinguishable across system and focused evidence views."
