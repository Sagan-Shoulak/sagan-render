#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
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

unset SAGAN_RENDER_AUTOCLOSE_MS SAGAN_RENDER_ELAPSED_SECONDS SAGAN_RENDER_TEST_KEY
unset SAGAN_RENDER_LOGICAL_WIDTH SAGAN_RENDER_LOGICAL_HEIGHT SAGAN_RENDER_UI_CAPTURE_BMP
export SAGAN_RENDER_UI_REQUIRE_EARTH=0
export SAGAN_RENDER_DEMO_KIND=loading
if [[ "$(uname -s)" == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
exec build/loading-sagan-demo/loading-sagan-demo
