#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/scene-sagan-demo-test

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
sagan_executable="${1:-${SAGAN_EXECUTABLE:-sagan}}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/scene_sagan_demo build/scene-sagan-demo-test/program.cpp
executable=build/scene-sagan-demo-test/scene-sagan-demo-test
warning_flags=()
compiler_version="$(c++ --version)"
if [[ "$compiler_version" == *clang* || "$compiler_version" == *Clang* ]]; then
  warning_flags+=(-Wno-parentheses-equality)
fi
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror "${warning_flags[@]}" \
  -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/scene-sagan-demo-test/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o "$executable"

case "$(uname -s)" in
  Darwin) export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"; expected=metal ;;
  *) export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"; expected=vulkan ;;
esac

report=build/scene-sagan-demo-test/scene-demo-report.txt
fingerprint() {
  local checksum bytes ignored
  read -r checksum bytes ignored < <(cksum "$1")
  printf '%s:%s' "$checksum" "$bytes"
}
rm -f "$report" build/scene-sagan-demo-test/scene-idle.bmp \
  build/scene-sagan-demo-test/scene-reframed.bmp \
  build/scene-sagan-demo-test/scene-camera-turned.bmp \
  build/scene-sagan-demo-test/scene-zoomed.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=500 SAGAN_RENDER_DEMO_KIND=scene
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-idle.bmp
unset SAGAN_RENDER_TEST_KEY
"$executable" | tee -a "$report"
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-camera-turned.bmp
export SAGAN_RENDER_TEST_ORBIT_DX=48 SAGAN_RENDER_TEST_ORBIT_DY=-24
"$executable" | tee -a "$report"
unset SAGAN_RENDER_TEST_ORBIT_DX SAGAN_RENDER_TEST_ORBIT_DY
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-zoomed.bmp
export SAGAN_RENDER_TEST_SCROLL_Y=2
"$executable" | tee -a "$report"
unset SAGAN_RENDER_TEST_SCROLL_Y
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-reframed.bmp
export SAGAN_RENDER_TEST_KEY=right
"$executable" | tee -a "$report"

grep -q "SAGAN_SCENE_DEMO language=sagan driver=$expected logical=960x540 precision_origin_metres=1e15 cleanup=1" "$report"
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-idle.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-reframed.bmp)" ]]; then
  echo "Camera reframe did not change the rendered scene" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-idle.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-camera-turned.bmp)" ]]; then
  echo "Right-drag orbit did not change the rendered 3D scene" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-idle.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-zoomed.bmp)" ]]; then
  echo "Mouse-wheel zoom did not change the rendered 3D scene" >&2
  exit 1
fi
echo "Sagan scene demo passed on $expected: smooth indexed bodies rendered; selection, right-drag orbit, and wheel zoom changed frame data."
