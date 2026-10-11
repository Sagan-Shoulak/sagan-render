#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
mkdir -p build/surface-sagan-demo

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
sagan_executable="$(bash scripts/resolve-sagan.sh)"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/surface_sagan_demo \
  build/surface-sagan-demo/program.cpp
executable="build/surface-sagan-demo/surface-sagan-demo"
warning_flags=()
compiler_version="$(c++ --version)"
if [[ "$compiler_version" == *clang* || "$compiler_version" == *Clang* ]]; then
  warning_flags+=(-Wno-parentheses-equality)
fi
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror "${warning_flags[@]}" \
  -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/surface-sagan-demo/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o "$executable"

unset SAGAN_RENDER_AUTOCLOSE_MS SAGAN_RENDER_UI_CAPTURE_BMP
export SAGAN_RENDER_DEMO_KIND=surface
if [[ "$(uname -s)" == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
exec "$executable"
