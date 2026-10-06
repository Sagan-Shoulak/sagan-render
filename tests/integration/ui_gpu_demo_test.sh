#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/ui-gpu-demo

sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
capture="build/ui-gpu-demo/ui-gpu-demo.bmp"
resize_capture="build/ui-gpu-demo/ui-gpu-demo-resized.bmp"
report="build/ui-gpu-demo/ui-gpu-demo-report.txt"
rm -f "$capture" "$resize_capture" "$report"

g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o build/ui-gpu-demo/ui-gpu-demo.exe
cp "$sdl_root/bin/SDL3.dll" build/ui-gpu-demo/SDL3.dll

export SAGAN_RENDER_AUTOCLOSE_MS=1500
export SAGAN_RENDER_START_PAUSED=1
export SAGAN_RENDER_UI_CAPTURE_BMP="$capture"
export SAGAN_RENDER_UI_RESIZE_CAPTURE_BMP="$resize_capture"
build/ui-gpu-demo/ui-gpu-demo.exe | tee "$report"

grep -q "SAGAN_UI_DEMO driver=direct3d12 initial_logical=960x540 solar_span_km=200000000 lunar_span_km=1000000 responsive=1" "$report"
grep -q "SAGAN_UI_PIXELS white=" "$report"
grep -Eq "modal=[5-9][0-9]{4,}" "$report"
grep -q "cleanup=1" "$report"
[[ -f "$capture" ]]
[[ "$(head -c 2 "$capture")" == "BM" ]]
dimensions="$(od -An -td4 -j18 -N8 "$capture" | tr -s ' ' | sed 's/^ //;s/ $//')"
[[ "$dimensions" == "960 540" ]]
resize_dimensions="$(od -An -td4 -j18 -N8 "$resize_capture" | tr -s ' ' | sed 's/^ //;s/ $//')"
[[ "$resize_dimensions" == "800 600" ]]

echo "SDL GPU UI demo passed on D3D12 with deterministic 960x540 and responsive 800x600 captures."
