#!/usr/bin/env bash
set -euo pipefail
export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/material-depth
sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
executable=build/material-depth/material-depth-probe.exe
capture=build/material-depth/material-depth.bmp
report=build/material-depth/material-depth-report.txt
rm -f "$executable" "$capture" "$report"
g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -I"$sdl_root/include" \
  tests/native/material_depth_probe.cpp -L"$sdl_root/lib" -lSDL3 -o "$executable"
cp "$sdl_root/bin/SDL3.dll" build/material-depth/SDL3.dll
export SAGAN_RENDER_MATERIAL_SHADER_DIR=shaders/generated/material
export SAGAN_RENDER_MATERIAL_CAPTURE_BMP="$capture"
"$executable" | tee "$report"
grep -q 'MATERIAL_DEPTH driver=direct3d12 shader=DXIL indexed=1 depth=1 uniforms=3' "$report"
grep -q 'cleanup=1' "$report"
[[ "$(head -c 2 "$capture")" == "BM" ]]
[[ "$(od -An -td4 -j18 -N8 "$capture" | tr -s ' ' | sed 's/^ //;s/ $//')" == "256 256" ]]
echo "Material depth probe passed on D3D12."
