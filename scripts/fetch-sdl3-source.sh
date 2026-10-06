#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

version="3.4.16"
asset="SDL3-${version}.tar.gz"
expected="7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68"
cache="build/dependencies"
archive="$cache/$asset"
source_directory="$cache/SDL3-${version}"
url="https://github.com/libsdl-org/SDL/releases/download/release-${version}/$asset"

mkdir -p "$cache"
if [[ ! -f "$archive" ]]; then
  curl --fail --location --output "$archive" "$url"
fi

if command -v sha256sum >/dev/null 2>&1; then
  actual="$(sha256sum "$archive" | cut -d ' ' -f1)"
else
  actual="$(shasum -a 256 "$archive" | cut -d ' ' -f1)"
fi
if [[ "$actual" != "$expected" ]]; then
  echo "SDL3 source checksum mismatch: expected $expected, got $actual" >&2
  exit 1
fi

if [[ ! -f "$source_directory/CMakeLists.txt" ]]; then
  rm -rf "$source_directory"
  tar -xzf "$archive" -C "$cache"
fi

[[ -f "$source_directory/include/SDL3/SDL.h" ]]
echo "$source_directory"
