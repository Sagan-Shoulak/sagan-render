#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

shader=shaders/post_process/tonemap.hlsl
manifest=shaders/post_process/tonemap.shader

grep -q 'SV_VertexID' "$shader"
grep -q 'register(t0, space2)' "$shader"
grep -q 'register(s0, space2)' "$shader"
grep -q 'register(b0, space3)' "$shader"
grep -q '1.0 - exp(-scene_linear \* exposure)' "$shader"
grep -q 'pow(mapped, inverse_gamma)' "$shader"
grep -q 'color_space = "linear-to-display"' "$manifest"
grep -q 'uniform_buffers = 0' "$manifest"
grep -q 'uniform_buffers = 1' "$manifest"
grep -q 'samplers = 1' "$manifest"
grep -q 'depth_test = false' "$manifest"
grep -q 'blend = false' "$manifest"

echo "Post-process contract passed: full-screen triangle, sampled scene color, tone mapping, and explicit render state verified."
