#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
mkdir -p build/toolbar-sagan-demo build/tmp
sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
sagan_executable="$(bash scripts/resolve-sagan.sh)"
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
unset SAGAN_RENDER_AUTOCLOSE_MS SAGAN_RENDER_TEST_KEY SAGAN_RENDER_UI_CAPTURE_BMP
unset SAGAN_RENDER_TEST_POINTER_X SAGAN_RENDER_TEST_POINTER_Y SAGAN_RENDER_TEST_POINTER_ACTION
unset SAGAN_RENDER_LOGICAL_WIDTH SAGAN_RENDER_LOGICAL_HEIGHT
export SAGAN_RENDER_UI_REQUIRE_EARTH=0 SAGAN_RENDER_DEMO_KIND=toolbar
exec build/toolbar-sagan-demo/toolbar-sagan-demo.exe
