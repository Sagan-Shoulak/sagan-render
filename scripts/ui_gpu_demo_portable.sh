#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
mkdir -p build/ui-gpu-demo

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
executable="build/ui-gpu-demo/ui-gpu-demo"
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o "$executable"

unset SAGAN_RENDER_AUTOCLOSE_MS
export SAGAN_RENDER_UI_CAPTURE_BMP="build/ui-gpu-demo/ui-gpu-demo.bmp"
if [[ "$(uname -s)" == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
exec "$executable"
