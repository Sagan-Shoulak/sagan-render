#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/ui-sagan-demo

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
sagan_executable="${SAGAN_EXECUTABLE:-sagan}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/ui_sagan_demo build/ui-sagan-demo/program.cpp

executable="build/ui-sagan-demo/ui-sagan-demo"
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/ui-sagan-demo/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o "$executable"

if [[ "$(uname -s)" == "Darwin" ]]; then
  expected="metal"
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  expected="vulkan"
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

report="build/ui-sagan-demo/ui-sagan-demo-report.txt"
rm -f "$report" build/ui-sagan-demo/ui-sagan-960x540.bmp build/ui-sagan-demo/ui-sagan-800x600.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=1800

export SAGAN_RENDER_LOGICAL_WIDTH=960 SAGAN_RENDER_LOGICAL_HEIGHT=540
export SAGAN_RENDER_UI_CAPTURE_BMP="build/ui-sagan-demo/ui-sagan-960x540.bmp"
"$executable" | tee -a "$report"

export SAGAN_RENDER_LOGICAL_WIDTH=800 SAGAN_RENDER_LOGICAL_HEIGHT=600
export SAGAN_RENDER_UI_CAPTURE_BMP="build/ui-sagan-demo/ui-sagan-800x600.bmp"
"$executable" | tee -a "$report"

grep -q "SAGAN_UI_DEMO language=sagan driver=$expected logical=960x540" "$report"
grep -q "SAGAN_UI_DEMO language=sagan driver=$expected logical=800x600" "$report"
grep -q "solar_span_km=200000000 lunar_span_km=1000000 cleanup=1" "$report"
[[ -f build/ui-sagan-demo/ui-sagan-960x540.bmp ]]
[[ -f build/ui-sagan-demo/ui-sagan-800x600.bmp ]]
echo "Sagan-authored GPU UI demo passed on $expected at two logical sizes."
