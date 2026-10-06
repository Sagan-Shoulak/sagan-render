#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/sdl-window

sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
capture="build/sdl-window/sdl-window.bmp"
report="build/sdl-window/sdl-window-report.txt"
rm -f "$capture" "$report"

g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" tests/native/sdl_window_probe.cpp \
  -L"$sdl_root/lib" -lSDL3 \
  -o build/sdl-window/sdl-window-probe.exe
cp "$sdl_root/bin/SDL3.dll" build/sdl-window/SDL3.dll

export SAGAN_RENDER_CAPTURE_BMP="$capture"
export SAGAN_RENDER_AUTOCLOSE_MS="${SAGAN_RENDER_AUTOCLOSE_MS:-1200}"
build/sdl-window/sdl-window-probe.exe | tee "$report"

grep -q "SDL_WINDOW_PROBE opened=1" "$report"
grep -q "SDL_WINDOW_PROBE closed=1 cleanup=1" "$report"
[[ -f "$capture" ]]
[[ "$(head -c 2 "$capture")" == "BM" ]]
dimensions="$(od -An -td4 -j18 -N8 "$capture" | tr -s ' ' | sed 's/^ //;s/ $//')"
[[ "$dimensions" == "960 540" ]]

echo "SDL3 window probe passed: visible lifecycle and 960x540 rendered BMP verified."
