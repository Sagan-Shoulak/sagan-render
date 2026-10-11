#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/surface-sagan-demo-test build/tmp

sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
sagan_executable="${1:-${SAGAN_EXECUTABLE:-sagan}}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/surface_sagan_demo \
  build/surface-sagan-demo-test/program.cpp
icon_resource="$($sagan_executable --application-icon windows)"
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/surface-sagan-demo-test/program.cpp examples/ui_gpu_demo.cpp \
  "$icon_resource" -L"$sdl_root/lib" -lSDL3 \
  -o build/surface-sagan-demo-test/surface-sagan-demo-test.exe
cp "$sdl_root/bin/SDL3.dll" build/surface-sagan-demo-test/SDL3.dll

capture="build/surface-sagan-demo-test/surface.bmp"
turned="build/surface-sagan-demo-test/surface-turned.bmp"
report="build/surface-sagan-demo-test/surface-demo-report.txt"
rm -f "$capture" "$turned" "$report"
export SAGAN_RENDER_AUTOCLOSE_MS=600 SAGAN_RENDER_DEMO_KIND=surface
export SAGAN_RENDER_UI_CAPTURE_BMP="$capture"
build/surface-sagan-demo-test/surface-sagan-demo-test.exe | tee -a "$report"
export SAGAN_RENDER_UI_CAPTURE_BMP="$turned" SAGAN_RENDER_TEST_ORBIT_DX=52
build/surface-sagan-demo-test/surface-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_ORBIT_DX

test -f "$capture"
test -f "$turned"
grep -q "SAGAN_SURFACE_DEMO language=sagan driver=direct3d12 logical=1100x700 precision_origin_metres=1e15 cleanup=1" "$report"
[[ "$(od -An -td4 -j18 -N8 "$capture" | tr -s ' ' | sed 's/^ //;s/ $//')" == "1100 700" ]]
if cmp -s "$capture" "$turned"; then
  echo "Surface orbit input did not change the rendered local scene" >&2
  exit 1
fi
echo "Sagan surface demo passed on D3D12: terrain, base, lander, depth, and orbit camera rendered."
