#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
mkdir -p build/lighting-demo
sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
sagan_executable="$(bash scripts/resolve-sagan.sh)"
icon_resource="$($sagan_executable --application-icon windows)"
g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" examples/lighting_demo.cpp \
  "$icon_resource" \
  -L"$sdl_root/lib" -lSDL3 -o build/lighting-demo/lighting-demo.exe
cp "$sdl_root/bin/SDL3.dll" build/lighting-demo/SDL3.dll
export SAGAN_RENDER_MATERIAL_SHADER_DIR=shaders/generated/material
exec build/lighting-demo/lighting-demo.exe
