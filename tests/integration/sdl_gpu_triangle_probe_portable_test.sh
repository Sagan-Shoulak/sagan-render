#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/sdl-gpu

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
shader_root="$(bash scripts/fetch-sdl-gpu-foundation-shaders.sh)"
capture="build/sdl-gpu/sdl-gpu-triangle.bmp"
report="build/sdl-gpu/sdl-gpu-report.txt"
executable="build/sdl-gpu/sdl-gpu-triangle-probe"
rm -f "$capture" "$report" "$executable"

c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" tests/native/sdl_gpu_triangle_probe.cpp \
  -L"$sdl_root/lib" -lSDL3 \
  -o "$executable"

export SAGAN_RENDER_SHADER_DIR="$shader_root"
export SAGAN_RENDER_GPU_CAPTURE_BMP="$capture"
if [[ "$(uname -s)" == "Darwin" ]]; then
  expected="driver=metal shader=MSL"
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  expected="driver=vulkan shader=SPIR-V"
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

"$executable" | tee "$report"

grep -q "SDL_GPU_PROBE device=1 $expected" "$report"
grep -q "command_buffer=1 render_pass=1 swapchain=1 triangle=1" "$report"
grep -q "SDL_GPU_PROBE cleanup=1" "$report"
[[ -f "$capture" ]]
[[ "$(head -c 2 "$capture")" == "BM" ]]

echo "SDL GPU triangle probe passed with deterministic pixel evidence."
