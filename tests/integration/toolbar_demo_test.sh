#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/toolbar-sagan-demo build/tmp
sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
sagan_executable="${SAGAN_EXECUTABLE:-sagan}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/toolbar_sagan_demo \
  build/toolbar-sagan-demo/program.cpp
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/toolbar-sagan-demo/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o build/toolbar-sagan-demo/toolbar-sagan-demo.exe
cp "$sdl_root/bin/SDL3.dll" build/toolbar-sagan-demo/SDL3.dll

export SAGAN_RENDER_AUTOCLOSE_MS=700 SAGAN_RENDER_UI_REQUIRE_EARTH=0
export SAGAN_RENDER_DEMO_KIND=toolbar
report=build/toolbar-sagan-demo/toolbar-demo-report.txt
rm -f "$report"
fingerprint() {
  local checksum bytes ignored
  read -r checksum bytes ignored < <(cksum "$1")
  printf '%s:%s' "$checksum" "$bytes"
}
require_same() {
  local description="$1" first="$2" second="$3"
  if [[ "$(fingerprint "$first")" != "$(fingerprint "$second")" ]]; then
    echo "$description should match: $first and $second" >&2
    return 1
  fi
}
require_different() {
  local description="$1" first="$2" second="$3"
  if [[ "$(fingerprint "$first")" == "$(fingerprint "$second")" ]]; then
    echo "$description should differ: $first and $second" >&2
    return 1
  fi
}
run_case() {
  local name="$1" width="$2" height="$3" key="${4:-}" x="${5:-}" y="${6:-}" action="${7:-}"
  export SAGAN_RENDER_LOGICAL_WIDTH="$width" SAGAN_RENDER_LOGICAL_HEIGHT="$height"
  export SAGAN_RENDER_TEST_KEY="$key" SAGAN_RENDER_TEST_POINTER_X="$x"
  export SAGAN_RENDER_TEST_POINTER_Y="$y" SAGAN_RENDER_TEST_POINTER_ACTION="$action"
  export SAGAN_RENDER_UI_CAPTURE_BMP="build/toolbar-sagan-demo/$name.bmp"
  build/toolbar-sagan-demo/toolbar-sagan-demo.exe | tee -a "$report"
  [[ "$(od -An -td4 -j18 -N8 "build/toolbar-sagan-demo/$name.bmp" | tr -s ' ' | sed 's/^ //;s/ $//')" == "$width $height" ]]
}
run_case idle-960x540 960 540
run_case keyboard-primary-960x540 960 540 enter
run_case focus-forward-960x540 960 540 right
run_case focus-reverse-960x540 960 540 shift-tab
run_case pointer-primary-960x540 960 540 "" 200 250 click
run_case disabled-960x540 960 540 "" 400 250 click
run_case hover-reset-960x540 960 540 "" 600 250 move
run_case pressed-reset-960x540 960 540 "" 600 250 down
run_case vertical-640x600 640 600
require_same "keyboard and pointer activation" \
  build/toolbar-sagan-demo/keyboard-primary-960x540.bmp \
  build/toolbar-sagan-demo/pointer-primary-960x540.bmp
require_same "forward and reverse focus traversal" \
  build/toolbar-sagan-demo/focus-forward-960x540.bmp \
  build/toolbar-sagan-demo/focus-reverse-960x540.bmp
require_different "idle and focused states" \
  build/toolbar-sagan-demo/idle-960x540.bmp \
  build/toolbar-sagan-demo/focus-forward-960x540.bmp
require_same "disabled activation" \
  build/toolbar-sagan-demo/idle-960x540.bmp \
  build/toolbar-sagan-demo/disabled-960x540.bmp
require_different "idle and hover states" \
  build/toolbar-sagan-demo/idle-960x540.bmp \
  build/toolbar-sagan-demo/hover-reset-960x540.bmp
require_different "hover and pressed states" \
  build/toolbar-sagan-demo/hover-reset-960x540.bmp \
  build/toolbar-sagan-demo/pressed-reset-960x540.bmp
grep -q "SAGAN_TOOLBAR_DEMO language=sagan driver=direct3d12 logical=960x540 cleanup=1" "$report"
grep -q "SAGAN_TOOLBAR_DEMO language=sagan driver=direct3d12 logical=640x600 cleanup=1" "$report"
echo "Sagan toolbar passed keyboard-pointer equivalence, disabled, hover, press, resize, and cleanup checks on D3D12."
