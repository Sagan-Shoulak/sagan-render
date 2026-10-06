#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/sdl-gpu

sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
shader_root="$(bash scripts/fetch-sdl-gpu-foundation-shaders.sh)"
capture="build/sdl-gpu/sdl-gpu-triangle.bmp"
report="build/sdl-gpu/sdl-gpu-report.txt"
rm -f "$capture" "$report"

g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" tests/native/sdl_gpu_triangle_probe.cpp \
  -L"$sdl_root/lib" -lSDL3 \
  -o build/sdl-gpu/sdl-gpu-triangle-probe.exe
cp "$sdl_root/bin/SDL3.dll" build/sdl-gpu/SDL3.dll

export SAGAN_RENDER_SHADER_DIR="$shader_root"
export SAGAN_RENDER_GPU_CAPTURE_BMP="$capture"
build/sdl-gpu/sdl-gpu-triangle-probe.exe | tee "$report"

grep -q "SDL_GPU_PROBE device=1 driver=direct3d12 shader=DXIL" "$report"
grep -q "command_buffer=1 render_pass=1 swapchain=1 triangle=1" "$report"
grep -q "SDL_GPU_PROBE cleanup=1" "$report"
[[ -f "$capture" ]]
[[ "$(head -c 2 "$capture")" == "BM" ]]
dimensions="$(od -An -td4 -j18 -N8 "$capture" | tr -s ' ' | sed 's/^ //;s/ $//')"
[[ "$dimensions" == "256 256" ]]

echo "SDL GPU triangle probe passed on D3D12 with deterministic pixel evidence."
