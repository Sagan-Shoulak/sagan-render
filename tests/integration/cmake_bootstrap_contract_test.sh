#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

metadata="$(bash scripts/resolve-cmake.sh --metadata)"
grep -q '^version=4.4.4$' <<< "$metadata"
grep -q '^asset=cmake-4.4.4-macos-universal.tar.gz$' <<< "$metadata"
grep -q '^sha256=4b7b73704b1db9b374e5c9ab8e17ac6148b817b6396ee75cd94a852cbac9d305$' \
  <<< "$metadata"
grep -q 'version = "4.4.4"' third_party/cmake-bootstrap.lock
grep -q 'macos_asset = "cmake-4.4.4-macos-universal.tar.gz"' third_party/cmake-bootstrap.lock
grep -q 'macos_sha256 = "4b7b73704b1db9b374e5c9ab8e17ac6148b817b6396ee75cd94a852cbac9d305"' \
  third_party/cmake-bootstrap.lock

echo "Pinned macOS CMake bootstrap contract passed: version, asset, and SHA-256 agree."
