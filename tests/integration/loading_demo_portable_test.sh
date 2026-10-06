#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/loading-sagan-demo
sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
sagan_executable="${SAGAN_EXECUTABLE:-sagan}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/loading_sagan_demo \
  build/loading-sagan-demo/program.cpp
warning_flags=()
compiler_version="$(c++ --version)"
if [[ "$compiler_version" == *clang* || "$compiler_version" == *Clang* ]]; then
  warning_flags+=(-Wno-parentheses-equality)
fi
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror "${warning_flags[@]}" \
  -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/loading-sagan-demo/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o build/loading-sagan-demo/loading-sagan-demo
if [[ "$(uname -s)" == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
  expected=metal
else
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  expected=vulkan
fi
export SAGAN_RENDER_AUTOCLOSE_MS=1800 SAGAN_RENDER_UI_REQUIRE_EARTH=0
export SAGAN_RENDER_DEMO_KIND=loading
report=build/loading-sagan-demo/loading-demo-report.txt
rm -f "$report"
run_case() {
  local name="$1" width="$2" height="$3" elapsed="$4" key="${5:-}" min_white="${6:-1000}"
  export SAGAN_RENDER_LOGICAL_WIDTH="$width" SAGAN_RENDER_LOGICAL_HEIGHT="$height"
  export SAGAN_RENDER_ELAPSED_SECONDS="$elapsed" SAGAN_RENDER_TEST_KEY="$key"
  export SAGAN_RENDER_UI_MIN_WHITE="$min_white"
  export SAGAN_RENDER_UI_CAPTURE_BMP="build/loading-sagan-demo/$name.bmp"
  build/loading-sagan-demo/loading-sagan-demo | tee -a "$report"
}
run_case loading-960x540 960 540 1.5
run_case ready-800x600 800 600 4.5
run_case transition-1024x576 1024 576 3.5 "" 0
run_case failed-960x540 960 540 1.5 f
grep -q "SAGAN_LOADING_DEMO language=sagan driver=$expected logical=960x540 cleanup=1" "$report"
grep -q "SAGAN_LOADING_DEMO language=sagan driver=$expected logical=800x600 cleanup=1" "$report"
grep -q "SAGAN_LOADING_DEMO language=sagan driver=$expected logical=1024x576 cleanup=1" "$report"
echo "Sagan loading-screen example passed loading, ready, failure, and responsive-layout checks on $expected."
