#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/material-contract build/tmp

compiler="${CXX:-c++}"
native_tmp="$repo_root/build/tmp"
if command -v cygpath >/dev/null 2>&1; then native_tmp="$(cygpath -w "$native_tmp")"; fi
TMPDIR="$native_tmp" TMP="$native_tmp" TEMP="$native_tmp" \
  "$compiler" -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  tests/native/material_contract_test.cpp \
  -o build/material-contract/material-contract-test
build/material-contract/material-contract-test

shader=shaders/material/lit_mesh.hlsl
manifest=shaders/material/lit_mesh.shader
grep -q 'register(b0, space1)' "$shader"
grep -q 'register(b0, space3)' "$shader"
grep -q 'register(b1, space3)' "$shader"
grep -q 'row_major float4x4 model_view_projection' "$shader"
grep -q 'color_space = "linear"' "$manifest"
grep -q 'uniform_buffers = 1' "$manifest"
grep -q 'uniform_buffers = 2' "$manifest"
echo "Shader interface passed: SDL GPU binding spaces, entries, and resource counts verified."
