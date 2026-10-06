#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

source_directory="$(bash scripts/fetch-sdl3-source.sh)"
build_directory="build/dependencies/SDL3-3.4.16-build"
install_directory="$repo_root/build/dependencies/SDL3-3.4.16-install"

cmake -S "$source_directory" -B "$build_directory" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$install_directory" \
  -DSDL_SHARED=ON \
  -DSDL_STATIC=OFF \
  -DSDL_TEST_LIBRARY=OFF \
  -DSDL_TESTS=OFF
cmake --build "$build_directory" --parallel 2
cmake --install "$build_directory"

[[ -f "$install_directory/include/SDL3/SDL.h" ]]
echo "$install_directory"
