#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

artifact_root="${1:-shaders/generated/post_process}"
for artifact in \
  tonemap.vert.dxil tonemap.vert.spv tonemap.vert.msl tonemap.vert.json \
  tonemap.frag.dxil tonemap.frag.spv tonemap.frag.msl tonemap.frag.json \
  tonemap.artifacts.sha256
do
  [[ -s "$artifact_root/$artifact" ]]
done

[[ "$(od -An -tc -N4 "$artifact_root/tonemap.vert.dxil" | tr -d ' ')" == "DXBC" ]]
[[ "$(od -An -tx1 -N4 "$artifact_root/tonemap.vert.spv" | tr -d ' \n')" == "03022307" ]]
[[ "$(od -An -tc -N4 "$artifact_root/tonemap.frag.dxil" | tr -d ' ')" == "DXBC" ]]
[[ "$(od -An -tx1 -N4 "$artifact_root/tonemap.frag.spv" | tr -d ' \n')" == "03022307" ]]

grep -q '#include <metal_stdlib>' "$artifact_root/tonemap.vert.msl"
grep -q '#include <metal_stdlib>' "$artifact_root/tonemap.frag.msl"
grep -q 'vertex' "$artifact_root/tonemap.vert.msl"
grep -q 'fragment' "$artifact_root/tonemap.frag.msl"

grep -q '"samplers": 0' "$artifact_root/tonemap.vert.json"
grep -q '"uniform_buffers": 0' "$artifact_root/tonemap.vert.json"
grep -q '"uniform_buffers": 1' "$artifact_root/tonemap.frag.json"
grep -q '"samplers": 1' "$artifact_root/tonemap.frag.json"

grep -q '^source_sha256=[0-9a-f]\{64\}$' "$artifact_root/tonemap.artifacts.sha256"
grep -q '^shadercross_commit=[0-9a-f]\{40\}$' "$artifact_root/tonemap.artifacts.sha256"
grep -q '^dxc_sha256=[0-9a-f]\{64\}$' "$artifact_root/tonemap.artifacts.sha256"

(
  cd "$artifact_root"
  tail -n 8 tonemap.artifacts.sha256 | while read -r expected filename; do
    if command -v shasum >/dev/null 2>&1; then
      actual="$(shasum -a 256 "$filename" | cut -d ' ' -f1)"
    else
      actual="$(sha256sum "$filename" | cut -d ' ' -f1)"
    fi
    [[ "$actual" == "$expected" ]]
  done
)

echo "Post-process artifacts passed: DXIL, SPIR-V, MSL, reflection, and provenance verified."
