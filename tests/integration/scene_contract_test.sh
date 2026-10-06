#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/scene-contract build/tmp
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
export TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp"

executable="build/scene-contract/scene-contract-test"
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) executable="${executable}.exe" ;;
esac

"${CXX:-c++}" -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  tests/native/scene_contract_test.cpp -o "$executable"
"$executable" build/scene-contract/scene-camera.svg

grep -q 'data-contract="sagan-scene-camera-v1"' build/scene-contract/scene-camera.svg
grep -q 'camera-relative projection near 1e15 metres' build/scene-contract/scene-camera.svg

sagan_executable="${1:-${SAGAN_EXECUTABLE:-sagan}}"
package_index="${SAGAN_PACKAGE_INDEX:-$repo_root/libraries/index.tsv}"
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  SAGAN_PACKAGE_INDEX="$package_index" "$sagan_executable" \
  --run-package tests/fixtures/scene_contract/sagan.toml

echo "Scene contract test passed: Sagan adapter, immutable snapshot, picking, selection, smooth focus, label placement, projection, visibility, and reframing verified."
