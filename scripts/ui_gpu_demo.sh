#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
mkdir -p build/ui-gpu-demo

sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o build/ui-gpu-demo/ui-gpu-demo.exe
cp "$sdl_root/bin/SDL3.dll" build/ui-gpu-demo/SDL3.dll

unset SAGAN_RENDER_AUTOCLOSE_MS
export SAGAN_RENDER_UI_CAPTURE_BMP="build/ui-gpu-demo/ui-gpu-demo.bmp"
exec build/ui-gpu-demo/ui-gpu-demo.exe
