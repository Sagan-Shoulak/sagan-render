#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/post-process
sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
executable=build/post-process/post-process-probe
capture=build/post-process/post-process.bmp
report=build/post-process/post-process-report.txt
rm -f "$executable" "$capture" "$report"
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -I"$sdl_root/include" \
  tests/native/post_process_probe.cpp -L"$sdl_root/lib" -lSDL3 -o "$executable"
export SAGAN_RENDER_POST_PROCESS_SHADER_DIR=shaders/generated/post_process
export SAGAN_RENDER_POST_PROCESS_CAPTURE_BMP="$capture"
if [[ "$(uname -s)" == "Darwin" ]]; then
  expected='driver=metal shader=MSL'
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  expected='driver=vulkan shader=SPIR-V'
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
"$executable" | tee "$report"
grep -q "POST_PROCESS $expected passes=2 sampled_scene=1 tonemap=1" "$report"
grep -q 'cleanup=1' "$report"
[[ "$(head -c 2 "$capture")" == "BM" ]]
echo "Post-process probe passed with $expected."
