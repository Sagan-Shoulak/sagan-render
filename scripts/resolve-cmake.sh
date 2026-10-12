#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

version=4.4.4
asset=cmake-4.4.4-macos-universal.tar.gz
expected=4b7b73704b1db9b374e5c9ab8e17ac6148b817b6396ee75cd94a852cbac9d305

if [[ "${1:-}" == --metadata ]]; then
  printf 'version=%s\nasset=%s\nsha256=%s\n' "$version" "$asset" "$expected"
  exit 0
fi

if [[ "${SAGAN_RENDER_CMAKE_BOOTSTRAP:-0}" != 1 ]] && command -v cmake >/dev/null 2>&1; then
  command -v cmake
  exit 0
fi

if [[ "$(uname -s)" != Darwin ]]; then
  echo "error: cmake is required; automatic CMake bootstrap currently supports macOS only" >&2
  exit 1
fi

cache="$repo_root/build/dependencies"
archive="$cache/$asset"
install="$cache/cmake-$version-macos-universal"
executable="$install/CMake.app/Contents/bin/cmake"
url="https://github.com/Kitware/CMake/releases/download/v$version/$asset"

mkdir -p "$cache"
if [[ ! -f "$archive" ]]; then
  curl --fail --location --output "$archive" "$url"
fi
actual="$(shasum -a 256 "$archive" | cut -d ' ' -f1)"
if [[ "$actual" != "$expected" ]]; then
  echo "CMake bootstrap checksum mismatch: expected $expected, got $actual" >&2
  exit 1
fi
if [[ ! -x "$executable" ]]; then
  rm -rf "$install"
  tar -xzf "$archive" -C "$cache"
fi
[[ -x "$executable" ]]
printf '%s\n' "$executable"
