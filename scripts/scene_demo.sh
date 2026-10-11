#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
mkdir -p build/scene-sagan-demo build/tmp

host_os="$(uname -s)"
case "$host_os" in
  MINGW*|MSYS*|CYGWIN*)
    export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
    ;;
esac

sagan_executable="$(bash scripts/resolve-sagan.sh)"
export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
"$sagan_executable" --emit-cpp-package examples/scene_sagan_demo build/scene-sagan-demo/program.cpp

common_sources=(build/scene-sagan-demo/program.cpp examples/ui_gpu_demo.cpp)
common_flags=(
  -std=c++23 -Wall -Wextra -Wpedantic -Werror -DSAGAN_RENDER_UI_BRIDGE
  -include "$repo_root/libraries/render/native/ui_gpu_bridge.hpp"
)

case "$host_os" in
  MINGW*|MSYS*|CYGWIN*)
    sdl_root="$(bash scripts/fetch-sdl3-windows.sh)"
    icon_resource="$($sagan_executable --application-icon windows)"
    native_tmp="$repo_root/build/tmp"
    if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
    TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
      g++ "${common_flags[@]}" -I"$sdl_root/include" "${common_sources[@]}" \
      "$icon_resource" -L"$sdl_root/lib" -lSDL3 \
      -o build/scene-sagan-demo/scene-sagan-demo.exe
    cp "$sdl_root/bin/SDL3.dll" build/scene-sagan-demo/SDL3.dll
    demo_executable=build/scene-sagan-demo/scene-sagan-demo.exe
    ;;
  Linux)
    if ! command -v pkg-config >/dev/null 2>&1; then
      echo "error: pkg-config is required to build the scene demo on Linux" >&2
      exit 1
    fi
    if ! pkg-config --atleast-version=3.4.16 sdl3; then
      echo "error: SDL3 3.4.16 or newer is required to build the scene demo on Linux" >&2
      echo "Install the SDL3 development package, then retry." >&2
      exit 1
    fi
    read -r -a sdl_cflags <<< "$(pkg-config --cflags sdl3)"
    read -r -a sdl_libs <<< "$(pkg-config --libs sdl3)"
    g++ "${common_flags[@]}" "${sdl_cflags[@]}" "${common_sources[@]}" \
      "${sdl_libs[@]}" -o build/scene-sagan-demo/scene-sagan-demo
    demo_executable=build/scene-sagan-demo/scene-sagan-demo
    ;;
  *)
    echo "error: scene_demo.sh currently supports Windows and Linux" >&2
    exit 1
    ;;
esac

unset SAGAN_RENDER_AUTOCLOSE_MS SAGAN_RENDER_TEST_KEY SAGAN_RENDER_UI_CAPTURE_BMP
export SAGAN_RENDER_DEMO_KIND=scene
exec "$demo_executable"
