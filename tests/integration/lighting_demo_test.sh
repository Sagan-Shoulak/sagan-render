#!/usr/bin/env bash
set -euo pipefail
export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/lighting-demo
capture=build/lighting-demo/lighting-demo.bmp
report=build/lighting-demo/lighting-demo-report.txt
rm -f "$capture" "$report"
export SAGAN_RENDER_AUTOCLOSE_MS=250
export SAGAN_RENDER_LIGHTING_CAPTURE_BMP="$capture"
bash scripts/lighting_demo.sh | tee "$report"
grep -q 'LIGHTING_DEMO driver=direct3d12 panels=3 lights=0,1,2 albedo=neutral-white' "$report"
grep -q 'cleanup=1' "$report"
[[ "$(head -c 2 "$capture")" == "BM" ]]
[[ "$(od -An -td4 -j18 -N8 "$capture" | tr -s ' ' | sed 's/^ //;s/ $//')" == "960 540" ]]
echo "Lighting demo passed on D3D12."
