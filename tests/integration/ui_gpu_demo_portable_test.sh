#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/ui-gpu-demo

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
capture="build/ui-gpu-demo/ui-gpu-demo.bmp"
resize_capture="build/ui-gpu-demo/ui-gpu-demo-resized.bmp"
report="build/ui-gpu-demo/ui-gpu-demo-report.txt"
executable="build/ui-gpu-demo/ui-gpu-demo"
rm -f "$capture" "$resize_capture" "$report" "$executable"

c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o "$executable"

export SAGAN_RENDER_AUTOCLOSE_MS=250
export SAGAN_RENDER_START_PAUSED=1
export SAGAN_RENDER_UI_CAPTURE_BMP="$capture"
export SAGAN_RENDER_UI_RESIZE_CAPTURE_BMP="$resize_capture"
if [[ "$(uname -s)" == "Darwin" ]]; then
  expected="driver=metal"
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  expected="driver=vulkan"
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

"$executable" | tee "$report"
grep -q "SAGAN_UI_DEMO $expected initial_logical=960x540 solar_span_km=200000000 lunar_span_km=1000000 responsive=1" "$report"
grep -q "SAGAN_UI_PIXELS white=" "$report"
grep -Eq "modal=[5-9][0-9]{4,}" "$report"
grep -q "cleanup=1" "$report"
[[ -f "$capture" ]]
[[ -f "$resize_capture" ]]
[[ "$(head -c 2 "$capture")" == "BM" ]]

echo "SDL GPU UI demo passed with a deterministic 960x540 capture."
