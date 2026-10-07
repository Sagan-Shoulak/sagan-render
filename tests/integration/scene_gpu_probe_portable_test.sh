#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/scene-3d
sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
executable=build/scene-3d/scene-gpu-probe
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" tests/native/scene_gpu_probe.cpp \
  -L"$sdl_root/lib" -lSDL3 -o "$executable"
case "$(uname -s)" in
  Darwin) export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"; expected=metal ;;
  *) export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"; expected=vulkan ;;
esac
export SAGAN_RENDER_MATERIAL_SHADER_DIR=shaders/generated/material
export SAGAN_RENDER_SCENE_3D_CAPTURE_BMP=build/scene-3d/scene-3d.bmp
"$executable" | tee build/scene-3d/scene-3d-report.txt
grep -q "SCENE_3D driver=$expected indexed=1 depth=1 views=2 bodies=3" \
  build/scene-3d/scene-3d-report.txt
echo "3D scene GPU probe passed on $expected: indexed Sun, Earth, and Moon models are distinguishable across physical system and lunar views."
