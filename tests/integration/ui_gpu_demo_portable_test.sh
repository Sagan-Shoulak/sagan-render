#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/ui-sagan-demo

sdl_root="$(bash scripts/build-sdl3-source.sh | tail -n 1)"
sagan_executable="${SAGAN_EXECUTABLE:-sagan}"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/ui_sagan_demo build/ui-sagan-demo/program.cpp

executable="build/ui-sagan-demo/ui-sagan-demo"
warning_flags=()
compiler_version="$(c++ --version)"
if [[ "$compiler_version" == *clang* || "$compiler_version" == *Clang* ]]; then
  # Remove with Sagan-Shoulak/sagan#8. Keep every other Clang warning fatal.
  warning_flags+=(-Wno-parentheses-equality)
fi
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror "${warning_flags[@]}" \
  -DSAGAN_RENDER_UI_BRIDGE \
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp" \
  -I"$sdl_root/include" build/ui-sagan-demo/program.cpp examples/ui_gpu_demo.cpp \
  -L"$sdl_root/lib" -lSDL3 -o "$executable"
launch_executable="$(bash scripts/stage-application-icon.sh "$sagan_executable" "$executable" \
  "Sagan UI Demo" "org.saganshoulak.render.ui-demo")"

if [[ "$(uname -s)" == "Darwin" ]]; then
  expected="metal"
  export DYLD_LIBRARY_PATH="$sdl_root/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
  [[ -f "build/ui-sagan-demo/application/Sagan UI Demo.app/Contents/Resources/sagan.icns" ]]
  grep -q '<key>CFBundleIconFile</key><string>sagan.icns</string>' \
    "build/ui-sagan-demo/application/Sagan UI Demo.app/Contents/Info.plist"
else
  expected="vulkan"
  export LD_LIBRARY_PATH="$sdl_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  [[ -f build/ui-sagan-demo/application/usr/share/icons/hicolor/512x512/apps/org-saganshoulak-render-ui-demo.png ]]
  grep -q '^Icon=org-saganshoulak-render-ui-demo$' \
    build/ui-sagan-demo/application/usr/share/applications/org-saganshoulak-render-ui-demo.desktop
fi

report="build/ui-sagan-demo/ui-sagan-demo-report.txt"
rm -f "$report" build/ui-sagan-demo/ui-sagan-960x540.bmp build/ui-sagan-demo/ui-sagan-800x600.bmp
export SAGAN_RENDER_AUTOCLOSE_MS=1800

export SAGAN_RENDER_LOGICAL_WIDTH=960 SAGAN_RENDER_LOGICAL_HEIGHT=540
export SAGAN_RENDER_UI_CAPTURE_BMP="build/ui-sagan-demo/ui-sagan-960x540.bmp"
"$launch_executable" | tee -a "$report"

export SAGAN_RENDER_LOGICAL_WIDTH=800 SAGAN_RENDER_LOGICAL_HEIGHT=600
export SAGAN_RENDER_UI_CAPTURE_BMP="build/ui-sagan-demo/ui-sagan-800x600.bmp"
"$launch_executable" | tee -a "$report"

grep -q "SAGAN_UI_DEMO language=sagan driver=$expected logical=960x540" "$report"
grep -q "SAGAN_UI_DEMO language=sagan driver=$expected logical=800x600" "$report"
grep -q "solar_span_km=200000000 lunar_span_km=1000000 cleanup=1" "$report"
[[ -f build/ui-sagan-demo/ui-sagan-960x540.bmp ]]
[[ -f build/ui-sagan-demo/ui-sagan-800x600.bmp ]]
echo "Sagan-authored GPU UI demo passed on $expected at two logical sizes."
