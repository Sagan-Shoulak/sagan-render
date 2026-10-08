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
icon_resource="$($sagan_executable --application-icon windows)"
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/scene-sagan-demo-test/program.cpp examples/ui_gpu_demo.cpp \
  "$icon_resource" -L"$sdl_root/lib" -lSDL3 -o build/scene-sagan-demo-test/scene-sagan-demo-test.exe
cp "$sdl_root/bin/SDL3.dll" build/scene-sagan-demo-test/SDL3.dll

report="build/scene-sagan-demo-test/scene-demo-report.txt"
fingerprint() {
  local checksum bytes ignored
  read -r checksum bytes ignored < <(cksum "$1")
  printf '%s:%s' "$checksum" "$bytes"
}
require_capture() {
  if [[ ! -f "$1" ]]; then
    echo "Expected scene capture was not created: $1" >&2
    exit 1
  fi
}
rm -f "$report" build/scene-sagan-demo-test/scene-idle.bmp \
  build/scene-sagan-demo-test/scene-top-locked.bmp \
  build/scene-sagan-demo-test/scene-top-overdrag.bmp \
  build/scene-sagan-demo-test/scene-selection-changed.bmp \
  build/scene-sagan-demo-test/scene-horizon.bmp \
  build/scene-sagan-demo-test/scene-underside.bmp \
  build/scene-sagan-demo-test/scene-focus-transition.bmp \
  build/scene-sagan-demo-test/scene-ancestor-transition.bmp \
  build/scene-sagan-demo-test/scene-moon-ancestor-transition.bmp \
  build/scene-sagan-demo-test/scene-marker.bmp \
  build/scene-sagan-demo-test/scene-marker-hovered.bmp \
  build/scene-sagan-demo-test/scene-marker-base.bmp \
  build/scene-sagan-demo-test/scene-camera-turned.bmp \
  build/scene-sagan-demo-test/scene-zoomed.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=500 SAGAN_RENDER_DEMO_KIND=scene
export SAGAN_RENDER_ELAPSED_SECONDS=0
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-idle.bmp
unset SAGAN_RENDER_TEST_KEY
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-top-locked.bmp
export SAGAN_RENDER_TEST_ORBIT_DY=1000
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-top-overdrag.bmp
export SAGAN_RENDER_TEST_ORBIT_DY=2000
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_ORBIT_DY
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-camera-turned.bmp
export SAGAN_RENDER_TEST_ORBIT_DX=48 SAGAN_RENDER_TEST_ORBIT_DY=-24
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_ORBIT_DX SAGAN_RENDER_TEST_ORBIT_DY
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-zoomed.bmp
export SAGAN_RENDER_TEST_SCROLL_Y=2
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_SCROLL_Y
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-horizon.bmp
export SAGAN_RENDER_TEST_ORBIT_DY=-87.26646259971647
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-underside.bmp
export SAGAN_RENDER_TEST_ORBIT_DY=-1000
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_ORBIT_DY
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-selection-changed.bmp
export SAGAN_RENDER_TEST_KEY=right
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"

unset SAGAN_RENDER_ELAPSED_SECONDS SAGAN_RENDER_TEST_KEY
export SAGAN_RENDER_TEST_KEYS=right,enter
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=700
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-focus-transition.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=1200
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_KEYS SAGAN_RENDER_UI_CAPTURE_AFTER_MS

export SAGAN_RENDER_TEST_KEYS=right,enter
export SAGAN_RENDER_TEST_DELAYED_KEYS=left,enter
export SAGAN_RENDER_TEST_DELAYED_KEYS_AFTER_MS=3300
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=4600
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-ancestor-transition.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=5100
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_KEYS SAGAN_RENDER_TEST_DELAYED_KEYS \
  SAGAN_RENDER_TEST_DELAYED_KEYS_AFTER_MS SAGAN_RENDER_UI_CAPTURE_AFTER_MS

export SAGAN_RENDER_TEST_KEYS=left,enter
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=3400
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-marker.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=3800
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"

export SAGAN_RENDER_TEST_POINTER_X=550 SAGAN_RENDER_TEST_POINTER_Y=220
export SAGAN_RENDER_TEST_POINTER_ACTION=move
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-marker-hovered.bmp
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_POINTER_ACTION

export SAGAN_RENDER_TEST_POINTER_X=550 SAGAN_RENDER_TEST_POINTER_Y=220
export SAGAN_RENDER_TEST_POINTER_AFTER_MS=4000
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=7200
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-marker-base.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=7600
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_KEYS SAGAN_RENDER_TEST_POINTER_X \
  SAGAN_RENDER_TEST_POINTER_Y SAGAN_RENDER_TEST_POINTER_AFTER_MS \
  SAGAN_RENDER_UI_CAPTURE_AFTER_MS

export SAGAN_RENDER_TEST_KEYS=left,enter
export SAGAN_RENDER_TEST_DELAYED_KEYS=right,enter
export SAGAN_RENDER_TEST_DELAYED_KEYS_AFTER_MS=3300
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=4600
export SAGAN_RENDER_UI_CAPTURE_BMP=build/scene-sagan-demo-test/scene-moon-ancestor-transition.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=5100
build/scene-sagan-demo-test/scene-sagan-demo-test.exe | tee -a "$report"
unset SAGAN_RENDER_TEST_KEYS SAGAN_RENDER_TEST_DELAYED_KEYS \
  SAGAN_RENDER_TEST_DELAYED_KEYS_AFTER_MS SAGAN_RENDER_UI_CAPTURE_AFTER_MS

require_capture build/scene-sagan-demo-test/scene-idle.bmp
require_capture build/scene-sagan-demo-test/scene-top-locked.bmp
require_capture build/scene-sagan-demo-test/scene-top-overdrag.bmp
require_capture build/scene-sagan-demo-test/scene-selection-changed.bmp
require_capture build/scene-sagan-demo-test/scene-horizon.bmp
require_capture build/scene-sagan-demo-test/scene-underside.bmp
require_capture build/scene-sagan-demo-test/scene-focus-transition.bmp
require_capture build/scene-sagan-demo-test/scene-ancestor-transition.bmp
require_capture build/scene-sagan-demo-test/scene-moon-ancestor-transition.bmp
require_capture build/scene-sagan-demo-test/scene-marker.bmp
require_capture build/scene-sagan-demo-test/scene-marker-hovered.bmp
require_capture build/scene-sagan-demo-test/scene-marker-base.bmp
require_capture build/scene-sagan-demo-test/scene-camera-turned.bmp
require_capture build/scene-sagan-demo-test/scene-zoomed.bmp
grep -q "SAGAN_SCENE_DEMO language=sagan driver=direct3d12 logical=960x540 precision_origin_metres=1e15 cleanup=1" "$report"
[[ "$(od -An -td4 -j18 -N8 build/scene-sagan-demo-test/scene-idle.bmp | tr -s ' ' | sed 's/^ //;s/ $//')" == "960 540" ]]
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-top-locked.bmp)" != \
      "$(fingerprint build/scene-sagan-demo-test/scene-top-overdrag.bmp)" ]]; then
  echo "Camera pitched past the world-up top pole" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-idle.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-selection-changed.bmp)" ]]; then
  echo "Selection did not change the rendered scene" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-idle.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-camera-turned.bmp)" ]]; then
  echo "Right-drag orbit did not change the rendered 3D scene" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-idle.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-focus-transition.bmp)" ]]; then
  echo "Staged focus transition did not change the rendered scene" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-horizon.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-underside.bmp)" ]]; then
  echo "Horizon and limited-underside camera views did not differ" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-idle.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-zoomed.bmp)" ]]; then
  echo "Mouse-wheel zoom did not change the rendered 3D scene" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-marker.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-marker-hovered.bmp)" ]]; then
  echo "Hovering the lunar surface marker did not fill its diamond" >&2
  exit 1
fi
if [[ "$(fingerprint build/scene-sagan-demo-test/scene-marker.bmp)" == \
      "$(fingerprint build/scene-sagan-demo-test/scene-marker-base.bmp)" ]]; then
  echo "Clicking the lunar surface marker did not enter the local base view" >&2
  exit 1
fi
echo "Sagan scene demo passed on D3D12: indexed bodies and lunar marker rendered; hover, local-base handoff, orbit, and zoom changed frame data."
