#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

version="3.4.16"
asset="SDL3-devel-${version}-mingw.zip"
expected="9828bb735cf8a007bcf0ac5aa9f01f3fcb54b7ca67c932e775c905c5d5053a60"
cache="build/dependencies"
archive="$cache/$asset"
destination="$cache/SDL3-${version}"
url="https://github.com/libsdl-org/SDL/releases/download/release-${version}/$asset"

mkdir -p "$cache"
if [[ ! -f "$archive" ]]; then
  curl --fail --location --output "$archive" "$url"
fi

actual="$(sha256sum "$archive" | cut -d ' ' -f1)"
if [[ "$actual" != "$expected" ]]; then
  echo "SDL3 archive checksum mismatch: expected $expected, got $actual" >&2
  exit 1
fi

if [[ ! -d "$destination/x86_64-w64-mingw32" ]]; then
  rm -rf "$destination"
  unzip -q "$archive" -d "$cache"
fi

[[ -f "$destination/x86_64-w64-mingw32/include/SDL3/SDL.h" ]]
[[ -f "$destination/x86_64-w64-mingw32/lib/libSDL3.dll.a" ]]
[[ -f "$destination/x86_64-w64-mingw32/bin/SDL3.dll" ]]

echo "$destination/x86_64-w64-mingw32"
