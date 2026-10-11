#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
demo_executable="${1:?Pass the freshly built scene demo executable}"
mkdir -p build/lunar-transition-test build/tmp
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
export TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp"
suffix=""
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) suffix=".exe" ;; esac
for test in lunar_surface lunar_transition_capture; do
  "${CXX:-c++}" -std=c++23 -Wall -Wextra -Wpedantic -Werror \
    "tests/native/${test}_test.cpp" -o "build/lunar-transition-test/${test}_test$suffix"
done
"build/lunar-transition-test/lunar_surface_test$suffix"
read -r source_hash ignored < <(sha256sum assets/planetary/moon_shackleton_rim_2048x2048.ppm)
[[ "$source_hash" == d6021bf68380c0b525e12d200aea39c78b077dc96f9b954e6466e58cda15c243 ]]

unset SAGAN_RENDER_ELAPSED_SECONDS SAGAN_RENDER_TEST_KEY SAGAN_RENDER_TEST_DELAYED_KEYS \
  SAGAN_RENDER_TEST_ORBIT_DX SAGAN_RENDER_TEST_ORBIT_DY SAGAN_RENDER_TEST_SCROLL_Y \
  SAGAN_RENDER_TEST_ORBIT_AFTER_MS SAGAN_RENDER_UI_CAPTURE_BMP SAGAN_RENDER_TEST_POINTER_ACTION \
  SAGAN_RENDER_TEST_INPUT_FILE SAGAN_RENDER_TEST_CAPTURE_TIMES_MS
export SAGAN_RENDER_DEMO_KIND=scene SAGAN_RENDER_TEST_KEYS=left,enter
export SAGAN_RENDER_TEST_INPUT_FILE=tests/fixtures/scene_test_size.txt
export SAGAN_RENDER_TEST_POINTER_X=550 SAGAN_RENDER_TEST_POINTER_Y=220
export SAGAN_RENDER_TEST_POINTER_AFTER_MS=4000 SAGAN_RENDER_TEST_STEP_MS=50
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=3900 SAGAN_RENDER_AUTOCLOSE_MS=10100
export SAGAN_RENDER_TEST_CAPTURE_PREFIX=build/lunar-transition-test/frame-
export SAGAN_RENDER_TEST_TRACE_SCENE=1 SAGAN_RENDER_UI_REQUIRE_EARTH=0
"$demo_executable" > build/lunar-transition-test/trace.txt
"build/lunar-transition-test/lunar_transition_capture_test$suffix" build/lunar-transition-test/trace.txt

# At the minimum local elevation, keep the near-horizon geometry and structures
# on the same surface. Also exercise a close zoom, not only the steep entry view.
export SAGAN_RENDER_TEST_ORBIT_DX=100 SAGAN_RENDER_TEST_ORBIT_DY=-300
export SAGAN_RENDER_TEST_SCROLL_Y=8 SAGAN_RENDER_TEST_ORBIT_AFTER_MS=9600
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=10000 SAGAN_RENDER_AUTOCLOSE_MS=10300
unset SAGAN_RENDER_TEST_CAPTURE_PREFIX SAGAN_RENDER_TEST_TRACE_SCENE
export SAGAN_RENDER_UI_CAPTURE_BMP=build/lunar-transition-test/local-horizon.bmp
"$demo_executable"
test -s build/lunar-transition-test/local-horizon.bmp
echo "Lunar continuity regression passed; dense transition and low-elevation close-view evidence retained."
