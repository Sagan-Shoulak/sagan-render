#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/ui-sagan-demo build/tmp

sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
sagan_executable="${SAGAN_EXECUTABLE:-sagan}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/ui_sagan_demo build/ui-sagan-demo/program.cpp

native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/ui-sagan-demo/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o build/ui-sagan-demo/ui-sagan-demo.exe
cp "$sdl_root/bin/SDL3.dll" build/ui-sagan-demo/SDL3.dll

report="build/ui-sagan-demo/ui-sagan-demo-report.txt"
rm -f "$report" build/ui-sagan-demo/ui-sagan-960x540.bmp build/ui-sagan-demo/ui-sagan-800x600.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=700

export SAGAN_RENDER_LOGICAL_WIDTH=960 SAGAN_RENDER_LOGICAL_HEIGHT=540
export SAGAN_RENDER_UI_CAPTURE_BMP="build/ui-sagan-demo/ui-sagan-960x540.bmp"
build/ui-sagan-demo/ui-sagan-demo.exe | tee -a "$report"

export SAGAN_RENDER_LOGICAL_WIDTH=800 SAGAN_RENDER_LOGICAL_HEIGHT=600
export SAGAN_RENDER_UI_CAPTURE_BMP="build/ui-sagan-demo/ui-sagan-800x600.bmp"
build/ui-sagan-demo/ui-sagan-demo.exe | tee -a "$report"

grep -q "SAGAN_UI_DEMO language=sagan driver=direct3d12 logical=960x540" "$report"
grep -q "SAGAN_UI_DEMO language=sagan driver=direct3d12 logical=800x600" "$report"
grep -q "solar_span_km=200000000 lunar_span_km=1000000 cleanup=1" "$report"
[[ "$(od -An -td4 -j18 -N8 build/ui-sagan-demo/ui-sagan-960x540.bmp | tr -s ' ' | sed 's/^ //;s/ $//')" == "960 540" ]]
[[ "$(od -An -td4 -j18 -N8 build/ui-sagan-demo/ui-sagan-800x600.bmp | tr -s ' ' | sed 's/^ //;s/ $//')" == "800 600" ]]
echo "Sagan-authored GPU UI demo passed on D3D12 at two logical sizes."
