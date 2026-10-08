#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/loading-sagan-demo build/tmp
sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
sagan_executable="${SAGAN_EXECUTABLE:-sagan}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/loading_sagan_demo \
  build/loading-sagan-demo/program.cpp
icon_resource="$($sagan_executable --application-icon windows)"
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/loading-sagan-demo/program.cpp examples/ui_gpu_demo.cpp \
  "$icon_resource" -L"$sdl_root/lib" -lSDL3 -o build/loading-sagan-demo/loading-sagan-demo.exe
cp "$sdl_root/bin/SDL3.dll" build/loading-sagan-demo/SDL3.dll

export SAGAN_RENDER_AUTOCLOSE_MS=700 SAGAN_RENDER_UI_REQUIRE_EARTH=0
export SAGAN_RENDER_DEMO_KIND=loading
report=build/loading-sagan-demo/loading-demo-report.txt
rm -f "$report"
run_case() {
  local name="$1" width="$2" height="$3" elapsed="$4" key="${5:-}" min_white="${6:-1000}"
  export SAGAN_RENDER_LOGICAL_WIDTH="$width" SAGAN_RENDER_LOGICAL_HEIGHT="$height"
  export SAGAN_RENDER_ELAPSED_SECONDS="$elapsed" SAGAN_RENDER_TEST_KEY="$key"
  export SAGAN_RENDER_UI_MIN_WHITE="$min_white"
  export SAGAN_RENDER_UI_CAPTURE_BMP="build/loading-sagan-demo/$name.bmp"
  build/loading-sagan-demo/loading-sagan-demo.exe | tee -a "$report"
  [[ "$(od -An -td4 -j18 -N8 "build/loading-sagan-demo/$name.bmp" | tr -s ' ' | sed 's/^ //;s/ $//')" == "$width $height" ]]
}
run_case loading-960x540 960 540 1.5
run_case ready-800x600 800 600 4.5
run_case transition-1024x576 1024 576 3.5 "" 0
run_case failed-960x540 960 540 1.5 f
grep -q "SAGAN_LOADING_DEMO language=sagan driver=direct3d12 logical=960x540 cleanup=1" "$report"
grep -q "SAGAN_LOADING_DEMO language=sagan driver=direct3d12 logical=800x600 cleanup=1" "$report"
grep -q "SAGAN_LOADING_DEMO language=sagan driver=direct3d12 logical=1024x576 cleanup=1" "$report"
echo "Sagan loading-screen example passed loading, ready, failure, and responsive-layout checks on D3D12."
