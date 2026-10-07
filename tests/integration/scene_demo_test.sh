#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/scene-sagan-demo-test build/tmp

sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
sagan_executable="${1:-${SAGAN_EXECUTABLE:-sagan}}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/scene_sagan_demo build/scene-sagan-demo-test/program.cpp
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/scene-sagan-demo-test/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o build/scene-sagan-demo-test/scene-sagan-demo-test.exe
cp "$sdl_root/bin/SDL3.dll" build/scene-sagan-demo-test/SDL3.dll

report="build/scene-sagan-demo-test/scene-demo-report.txt"
rm -f "$report" build/scene-sagan-demo-test/scene-idle.bmp \
  build/scene-sagan-demo-test/scene-reframed.bmp \
  build/scene-sagan-demo-test/scene-camera-turned.bmp \
  build/scene-sagan-demo-test/scene-zoomed.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=500 SAGAN_RENDER_DEMO_KIND=scene
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-idle.bmp
unset SAGAN_RENDER_TEST_KEY
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-camera-turned.bmp
export SAGAN_RENDER_TEST_ORBIT_DX=48 SAGAN_RENDER_TEST_ORBIT_DY=-24
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_ORBIT_DX SAGAN_RENDER_TEST_ORBIT_DY
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-zoomed.bmp
export SAGAN_RENDER_TEST_SCROLL_Y=2
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_SCROLL_Y
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-reframed.bmp
export SAGAN_RENDER_TEST_KEY=right
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"

grep -q "SAGAN_SCENE_DEMO language=sagan driver=direct3d12 logical=960x540 precision_origin_metres=1e15 cleanup=1" "$report"
[[ "$(od -An -td4 -j18 -N8 build/scene-sagan-demo-test/scene-idle.bmp | tr -s ' ' | sed 's/^ //;s/ $//')" == "960 540" ]]
if cmp -s build/scene-sagan-demo-test/scene-idle.bmp build/scene-sagan-demo-test/scene-reframed.bmp; then
  echo "Camera reframe did not change the rendered scene" >&2
  exit 1
fi
if cmp -s build/scene-sagan-demo-test/scene-idle.bmp build/scene-sagan-demo-test/scene-camera-turned.bmp; then
  echo "Right-drag orbit did not change the rendered 3D scene" >&2
  exit 1
fi
if cmp -s build/scene-sagan-demo-test/scene-idle.bmp build/scene-sagan-demo-test/scene-zoomed.bmp; then
  echo "Mouse-wheel zoom did not change the rendered 3D scene" >&2
  exit 1
fi
echo "Sagan scene demo passed on D3D12: smooth indexed bodies rendered; selection, right-drag orbit, and wheel zoom changed frame data."
