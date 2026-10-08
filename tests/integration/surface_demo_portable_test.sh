#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/surface-sagan-demo-test

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
sagan_executable="${1:-${SAGAN_EXECUTABLE:-sagan}}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/surface_sagan_demo \
  build/surface-sagan-demo-test/program.cpp
executable=build/surface-sagan-demo-test/surface-sagan-demo-test
warning_flags=()
compiler_version="$(c++ --version)"
if [[ "$compiler_version" == *clang* || "$compiler_version" == *Clang* ]]; then
  warning_flags+=(-Wno-parentheses-equality)
fi
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror "${warning_flags[@]}" \
  -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/surface-sagan-demo-test/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o "$executable"

case "$(uname -s)" in
  Darwin) export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"; expected=metal ;;
  *) export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"; expected=vulkan ;;
esac

capture=build/surface-sagan-demo-test/surface.bmp
turned=build/surface-sagan-demo-test/surface-turned.bmp
report=build/surface-sagan-demo-test/surface-demo-report.txt
rm -f "$capture" "$turned" "$report"
export SAGAN_RENDER_AUTOCLOSE_MS=600 SAGAN_RENDER_DEMO_KIND=surface
export SAGAN_RENDER_UI_CAPTURE_BMP="$capture"
"$executable" | tee -a "$report"
export SAGAN_RENDER_UI_CAPTURE_BMP="$turned" SAGAN_RENDER_TEST_ORBIT_DX=52
"$executable" | tee -a "$report"
unset SAGAN_RENDER_TEST_ORBIT_DX

grep -q "SAGAN_SURFACE_DEMO language=sagan driver=$expected logical=1100x700 precision_origin_metres=1e15 cleanup=1" "$report"
if cmp -s "$capture" "$turned"; then
  echo "Surface orbit input did not change the rendered local scene" >&2
  exit 1
fi
echo "Sagan surface demo passed on $expected: terrain, base, lander, depth, and orbit camera rendered."
