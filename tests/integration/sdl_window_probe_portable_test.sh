#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/sdl-window

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
capture="build/sdl-window/sdl-window.bmp"
report="build/sdl-window/sdl-window-report.txt"
executable="build/sdl-window/sdl-window-probe"
rm -f "$capture" "$report" "$executable"

c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -I"$sdl_root/include" tests/native/sdl_window_probe.cpp \
  -L"$sdl_root/lib" -lSDL3 \
  -o "$executable"

export SAGAN_RENDER_CAPTURE_BMP="$capture"
export SAGAN_RENDER_AUTOCLOSE_MS="${SAGAN_RENDER_AUTOCLOSE_MS:-1200}"
if [[ "$(uname -s)" == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

"$executable" | tee "$report"

grep -q "SDL_WINDOW_PROBE opened=1" "$report"
grep -q "SDL_WINDOW_PROBE closed=1 cleanup=1" "$report"
[[ -f "$capture" ]]
[[ "$(head -c 2 "$capture")" == "BM" ]]

echo "SDL3 portable window probe passed: lifecycle and rendered BMP verified."
