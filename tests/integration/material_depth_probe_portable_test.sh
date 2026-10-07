#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/material-depth
sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
executable=build/material-depth/material-depth-probe
capture=build/material-depth/material-depth.bmp
report=build/material-depth/material-depth-report.txt
rm -f "$executable" "$capture" "$report"
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -I"$sdl_root/include" \
  tests/native/material_depth_probe.cpp -L"$sdl_root/lib" -lSDL3 -o "$executable"
export SAGAN_RENDER_MATERIAL_SHADER_DIR=shaders/generated/material
export SAGAN_RENDER_MATERIAL_CAPTURE_BMP="$capture"
if [[ "$(uname -s)" == "Darwin" ]]; then
  expected='driver=metal shader=MSL'
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  expected='driver=vulkan shader=SPIR-V'
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
"$executable" | tee "$report"
grep -q "MATERIAL_DEPTH $expected indexed=1 depth=1 uniforms=3" "$report"
grep -q 'cleanup=1' "$report"
[[ "$(head -c 2 "$capture")" == "BM" ]]
echo "Material depth probe passed with $expected."
