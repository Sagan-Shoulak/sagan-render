#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

mkdir -p build/window-demo build/tmp

export SAGAN_PACKAGE_INDEX="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
sagan_executable="$(bash scripts/resolve-sagan.sh)"

"$sagan_executable" --emit-cpp-package examples/window_demo build/window-demo/program.cpp

native_output="build/window-demo/window-demo"
if [[ "${OS:-}" == "Windows_NT" ]]; then
  native_output="build/window-demo/window-demo.exe"
fi

native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then
  native_tmp="$(cygpath -w "$native_tmp")"
fi

TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  -include "$repo_root/libraries/render/native/window_bridge.hpp" \
  build/window-demo/program.cpp libraries/render/native/window_bridge.cpp \
  -o "$native_output" -static -static-libgcc -static-libstdc++ -lgdi32 -luser32

"$native_output"
