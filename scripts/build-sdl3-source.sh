#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

source_directory="$(bash scripts/fetch-sdl3-source.sh)"
cmake_executable="$(bash scripts/resolve-cmake.sh)"
generator="$(bash scripts/select-cmake-generator.sh)"
case "$generator" in
  Ninja) generator_key=ninja ;;
  "Unix Makefiles") generator_key=make ;;
esac
build_directory="build/dependencies/SDL3-3.4.16-build-$generator_key"
install_directory="$repo_root/build/dependencies/SDL3-3.4.16-install"

"$cmake_executable" -S "$source_directory" -B "$build_directory" -G "$generator" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$install_directory" \
  -DSDL_SHARED=ON \
  -DSDL_STATIC=OFF \
  -DSDL_TEST_LIBRARY=OFF \
  -DSDL_TESTS=OFF
"$cmake_executable" --build "$build_directory" --parallel 2
"$cmake_executable" --install "$build_directory"

[[ -f "$install_directory/include/SDL3/SDL.h" ]]
echo "$install_directory"
