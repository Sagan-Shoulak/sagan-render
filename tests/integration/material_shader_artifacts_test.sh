#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

artifact_root="${1:-shaders/generated/material}"
for artifact in \
  lit_mesh.vert.dxil lit_mesh.vert.spv lit_mesh.vert.msl lit_mesh.vert.json \
  lit_mesh.frag.dxil lit_mesh.frag.spv lit_mesh.frag.msl lit_mesh.frag.json \
  lit_mesh.artifacts.sha256
do
  [[ -s "$artifact_root/$artifact" ]]
done

[[ "$(od -An -tc -N4 "$artifact_root/lit_mesh.vert.dxil" | tr -d ' ')" == "DXBC" ]]
[[ "$(od -An -tx1 -N4 "$artifact_root/lit_mesh.vert.spv" | tr -d ' \n')" == "03022307" ]]
[[ "$(od -An -tc -N4 "$artifact_root/lit_mesh.frag.dxil" | tr -d ' ')" == "DXBC" ]]
[[ "$(od -An -tx1 -N4 "$artifact_root/lit_mesh.frag.spv" | tr -d ' \n')" == "03022307" ]]

grep -q '#include <metal_stdlib>' "$artifact_root/lit_mesh.vert.msl"
grep -q '#include <metal_stdlib>' "$artifact_root/lit_mesh.frag.msl"
grep -q 'vertex' "$artifact_root/lit_mesh.vert.msl"
grep -q 'fragment' "$artifact_root/lit_mesh.frag.msl"

grep -q '"samplers": 0' "$artifact_root/lit_mesh.vert.json"
grep -q '"storage_textures": 0' "$artifact_root/lit_mesh.vert.json"
grep -q '"storage_buffers": 0' "$artifact_root/lit_mesh.vert.json"
grep -q '"uniform_buffers": 1' "$artifact_root/lit_mesh.vert.json"
grep -q '"uniform_buffers": 2' "$artifact_root/lit_mesh.frag.json"
grep -q '"location": 0' "$artifact_root/lit_mesh.vert.json"
grep -q '"location": 1' "$artifact_root/lit_mesh.vert.json"

grep -q '^source_sha256=[0-9a-f]\{64\}$' "$artifact_root/lit_mesh.artifacts.sha256"
grep -q '^shadercross_commit=[0-9a-f]\{40\}$' "$artifact_root/lit_mesh.artifacts.sha256"
grep -q '^dxc_sha256=[0-9a-f]\{64\}$' "$artifact_root/lit_mesh.artifacts.sha256"

(
  cd "$artifact_root"
  tail -n 8 lit_mesh.artifacts.sha256 | sha256sum -c >/dev/null
)

echo "Material artifacts passed: DXIL and SPIR-V magic, MSL stages, reflected resources, and provenance verified."
