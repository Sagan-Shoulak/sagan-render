#!/usr/bin/env bash
set -euo pipefail
export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
demo_executable="${1:?Pass the freshly built scene demo executable}"
mkdir -p build/earth-zoom-test build/tmp
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
export TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp"
suffix=""
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) suffix=".exe" ;; esac
"${CXX:-c++}" -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  tests/native/lunar_anchor_projection_test.cpp -o "build/earth-zoom-test/anchor-test$suffix"
"build/earth-zoom-test/anchor-test$suffix"
unset SAGAN_RENDER_ELAPSED_SECONDS SAGAN_RENDER_TEST_KEY SAGAN_RENDER_TEST_DELAYED_KEYS \
  SAGAN_RENDER_TEST_POINTER_X SAGAN_RENDER_TEST_POINTER_Y SAGAN_RENDER_TEST_POINTER_AFTER_MS \
  SAGAN_RENDER_TEST_POINTER_ACTION SAGAN_RENDER_TEST_ORBIT_DX SAGAN_RENDER_TEST_ORBIT_DY \
  SAGAN_RENDER_TEST_SCROLL_Y SAGAN_RENDER_TEST_ORBIT_AFTER_MS SAGAN_RENDER_UI_CAPTURE_BMP \
  SAGAN_RENDER_TEST_STOP_AFTER_MS SAGAN_RENDER_TEST_TRACE_SCENE \
  SAGAN_RENDER_TEST_LATE_AFTER_MS SAGAN_RENDER_TEST_LATE_STEP_MS
export SAGAN_RENDER_DEMO_KIND=scene SAGAN_RENDER_TEST_KEYS=right,enter
export SAGAN_RENDER_TEST_INPUT_FILE=tests/fixtures/scene_earth_zoom_input.txt
export SAGAN_RENDER_TEST_STEP_MS=50 SAGAN_RENDER_AUTOCLOSE_MS=4750
export SAGAN_RENDER_TEST_CAPTURE_PREFIX=build/earth-zoom-test/frame-
export SAGAN_RENDER_TEST_CAPTURE_TIMES_MS=4400,4450,4500
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=4400 SAGAN_RENDER_UI_REQUIRE_EARTH=1
export SAGAN_RENDER_TEST_TRACE_SCENE=1
# Reject stale captures so the formerly failing frame must be produced anew.
rm -f build/earth-zoom-test/frame-4400.bmp build/earth-zoom-test/frame-4450.bmp build/earth-zoom-test/frame-4500.bmp
"$demo_executable" > build/earth-zoom-test/report.txt
for frame in 4400 4450 4500; do test -s "build/earth-zoom-test/frame-$frame.bmp"; done
grep -q 'SAGAN_UI_PIXELS white=.*earth=' build/earth-zoom-test/report.txt
grep -q 'SAGAN_SCENE_DEMO .*logical=960x540.*cleanup=1' build/earth-zoom-test/report.txt
test "$(grep -c '^SAGAN_SCENE_DRAW id=2 .* near=1000000 ' build/earth-zoom-test/report.txt)" -eq 3
"build/earth-zoom-test/anchor-test$suffix" build/earth-zoom-test
echo "Earth zoom regression passed: before/during/after rear lunar-origin crossing rendered and closed cleanly."
