#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

generator="$(bash scripts/select-cmake-generator.sh)"
case "$generator" in
  Ninja|"Unix Makefiles") ;;
  *)
    echo "Unexpected automatic generator: $generator" >&2
    exit 1
    ;;
esac

if command -v make >/dev/null 2>&1; then
  [[ "$(SAGAN_RENDER_CMAKE_GENERATOR='Unix Makefiles' bash scripts/select-cmake-generator.sh)" == \
      "Unix Makefiles" ]]
fi
if command -v ninja >/dev/null 2>&1; then
  [[ "$(SAGAN_RENDER_CMAKE_GENERATOR=Ninja bash scripts/select-cmake-generator.sh)" == Ninja ]]
fi

if SAGAN_RENDER_CMAKE_GENERATOR=unsupported bash scripts/select-cmake-generator.sh \
    >build/cmake-generator-invalid.out 2>&1; then
  echo "Unsupported generator was accepted" >&2
  exit 1
fi
grep -q "unsupported SDL CMake generator" build/cmake-generator-invalid.out
rm -f build/cmake-generator-invalid.out

echo "SDL CMake generator selection passed: automatic=$generator; Make and Ninja are explicit reviewed options."
