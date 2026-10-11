#!/usr/bin/env bash
set -euo pipefail
export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
demo_executable="${1:?Pass the freshly built scene demo executable}"
generated_source="${2:?Pass its emitted program.cpp}"
mkdir -p build/scene-base-ui-test build/tmp
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
export TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp"
suffix=""
link_flags=()
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    suffix=".exe"
    sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
    link_flags=(-L"$sdl_root/lib" -lSDL3)
    ;;
  *)
    sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
    link_flags=(-L"$sdl_root/lib" -lSDL3)
    if [[ "$(uname -s)" != Darwin ]]; then link_flags+=(-Wl,-rpath,"$sdl_root/lib"); fi
    ;;
esac
compiler_flags=()
compiler_version="$("${CXX:-c++}" --version)"
if [[ "$compiler_version" == *clang* || "$compiler_version" == *Clang* ]]; then
  compiler_flags+=(-Wno-parentheses-equality)
fi
"${CXX:-c++}" -std=c++23 -Wall -Wextra -Wpedantic -Werror "${compiler_flags[@]}" \
  -DSAGAN_RENDER_UI_BRIDGE -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$(dirname "$generated_source")" -I"$sdl_root/include" \
  tests/native/scene_base_ui_state_test.cpp examples/ui_gpu_demo.cpp \
  "${link_flags[@]}" -o "build/scene-base-ui-test/state-test$suffix"
if [[ -n "$suffix" ]]; then cp "$sdl_root/bin/SDL3.dll" build/scene-base-ui-test/SDL3.dll; fi
"build/scene-base-ui-test/state-test$suffix"
"${CXX:-c++}" -std=c++23 -Wall -Wextra -Wpedantic -Werror "${compiler_flags[@]}" \
  -DSAGAN_RENDER_UI_BRIDGE -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$(dirname "$generated_source")" -I"$sdl_root/include" \
  tests/native/scene_base_economy_test.cpp examples/ui_gpu_demo.cpp \
  "${link_flags[@]}" -o "build/scene-base-ui-test/economy-test$suffix"
"build/scene-base-ui-test/economy-test$suffix"

unset SAGAN_RENDER_ELAPSED_SECONDS SAGAN_RENDER_TEST_KEY SAGAN_RENDER_TEST_KEYS \
  SAGAN_RENDER_TEST_DELAYED_KEYS SAGAN_RENDER_TEST_POINTER_X SAGAN_RENDER_TEST_POINTER_Y \
  SAGAN_RENDER_TEST_POINTER_AFTER_MS SAGAN_RENDER_TEST_POINTER_ACTION \
  SAGAN_RENDER_TEST_ORBIT_DX SAGAN_RENDER_TEST_ORBIT_DY SAGAN_RENDER_TEST_SCROLL_Y \
  SAGAN_RENDER_TEST_ORBIT_AFTER_MS SAGAN_RENDER_UI_CAPTURE_BMP SAGAN_RENDER_TEST_TRACE_SCENE
export SAGAN_RENDER_DEMO_KIND=scene SAGAN_RENDER_TEST_KEYS=left,enter
export SAGAN_RENDER_TEST_INPUT_FILE=tests/fixtures/scene_base_ui_input.txt
export SAGAN_RENDER_TEST_STEP_MS=50 SAGAN_RENDER_UI_REQUIRE_EARTH=0
export SAGAN_RENDER_UI_CAPTURE_AFTER_MS=9400 SAGAN_RENDER_AUTOCLOSE_MS=16150
export SAGAN_RENDER_TEST_CAPTURE_PREFIX=build/scene-base-ui-test/frame-
export SAGAN_RENDER_TEST_CAPTURE_TIMES_MS=9400,9500,9600,9800,9850,9900,10000,10100,10550,10600,16100
"$demo_executable" > build/scene-base-ui-test/ui-report.txt
for frame in 9400 9500 9600 9800 9850 9900 10000 10100 10550 10600 16100; do
  test -s "build/scene-base-ui-test/frame-$frame.bmp"
done
echo "Scene base UI interaction sequence passed; deterministic panel/build/return captures retained."
export SAGAN_RENDER_TEST_INPUT_FILE=tests/fixtures/scene_base_ui_keyboard.txt
export SAGAN_RENDER_TEST_CAPTURE_PREFIX=build/scene-base-ui-test/keyboard-
export SAGAN_RENDER_TEST_CAPTURE_TIMES_MS=10400,10600,10650,10700
export SAGAN_RENDER_AUTOCLOSE_MS=10800
"$demo_executable" > build/scene-base-ui-test/keyboard-report.txt
export SAGAN_RENDER_TEST_INPUT_FILE=tests/fixtures/scene_base_ui_small.txt
export SAGAN_RENDER_TEST_CAPTURE_PREFIX=build/scene-base-ui-test/small-
export SAGAN_RENDER_TEST_CAPTURE_TIMES_MS=10000
export SAGAN_RENDER_AUTOCLOSE_MS=10100
"$demo_executable" > build/scene-base-ui-test/small-report.txt
test -s build/scene-base-ui-test/keyboard-10400.bmp
test -s build/scene-base-ui-test/small-10000.bmp
echo "Scene base UI keyboard and 800x400 scrolling/confirmation captures retained."
export SAGAN_RENDER_TEST_INPUT_FILE=tests/fixtures/scene_base_economy_input.txt
export SAGAN_RENDER_TEST_CAPTURE_PREFIX=build/scene-base-ui-test/economy-
export SAGAN_RENDER_TEST_CAPTURE_TIMES_MS=9500,12500,13000,17500,19000,19750,19500,20000,22000,40000,45000,65000,70000,130000,660000
export SAGAN_RENDER_TEST_STEP_MS=250 SAGAN_RENDER_AUTOCLOSE_MS=660250
"$demo_executable" > build/scene-base-ui-test/economy-report.txt
for frame in 9500 12500 13000 17500 19000 19750 19500 20000 22000 40000 45000 65000 70000 130000 660000; do
  test -s "build/scene-base-ui-test/economy-$frame.bmp"
done
echo "Scene base economy bulk-build, shortage, power, food, production and automatic maintenance captures retained."
export SAGAN_RENDER_TEST_INPUT_FILE=tests/fixtures/scene_base_context_input.txt
export SAGAN_RENDER_TEST_CAPTURE_PREFIX=build/scene-base-ui-test/context-
export SAGAN_RENDER_TEST_CAPTURE_TIMES_MS=70000,120000,121250,122250,124250,125250,127250,128250,129250,131250,132250,133250,135250,136250,137250,139250,140250,142250,143250,144250,146250,147250,148250,150250,151250,154250,155250
export SAGAN_RENDER_AUTOCLOSE_MS=155500
"$demo_executable" > build/scene-base-ui-test/context-report.txt
for frame in 70000 120000 121250 122250 124250 125250 127250 128250 129250 131250 132250 133250 135250 136250 137250 139250 140250 142250 143250 144250 146250 147250 148250 150250 151250 154250 155250; do
  test -s "build/scene-base-ui-test/context-$frame.bmp"
done
echo "All nine live system inspectors and scrolling captures retained."
"${CXX:-c++}" -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  tests/native/scene_base_ui_capture_test.cpp -o "build/scene-base-ui-test/capture-test$suffix"
"build/scene-base-ui-test/capture-test$suffix" build/scene-base-ui-test
