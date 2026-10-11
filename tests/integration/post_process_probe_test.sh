#!/usr/bin/env bash
set -euo pipefail
export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/post-process
sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
executable=build/post-process/post-process-probe.exe
capture=build/post-process/post-process.bmp
report=build/post-process/post-process-report.txt
rm -f "$executable" "$capture" "$report"
g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -I"$sdl_root/include" \
  tests/native/post_process_probe.cpp -L"$sdl_root/lib" -lSDL3 -o "$executable"
cp "$sdl_root/bin/SDL3.dll" build/post-process/SDL3.dll
export SAGAN_RENDER_POST_PROCESS_SHADER_DIR=shaders/generated/post_process
export SAGAN_RENDER_POST_PROCESS_CAPTURE_BMP="$capture"
"$executable" | tee "$report"
grep -q 'POST_PROCESS driver=direct3d12 shader=DXIL passes=2 sampled_scene=1 tonemap=1' "$report"
grep -q 'cleanup=1' "$report"
[[ "$(head -c 2 "$capture")" == "BM" ]]
[[ "$(od -An -td4 -j18 -N8 "$capture" | tr -s ' ' | sed 's/^ //;s/ $//')" == "256 256" ]]
echo "Post-process probe passed on D3D12."
