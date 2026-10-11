#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/lighting-demo
capture=build/lighting-demo/lighting-demo.bmp
report=build/lighting-demo/lighting-demo-report.txt
rm -f "$capture" "$report"
export SAGAN_RENDER_AUTOCLOSE_MS=250
export SAGAN_RENDER_LIGHTING_CAPTURE_BMP="$capture"
if [[ "$(uname -s)" == "Darwin" ]]; then
  expected=metal
else
  expected=vulkan
fi
bash scripts/lighting_demo_portable.sh | tee "$report"
grep -q "LIGHTING_DEMO driver=$expected panels=3 lights=0,1,2" "$report"
grep -q 'cleanup=1' "$report"
[[ "$(head -c 2 "$capture")" == "BM" ]]
echo "Lighting demo passed on $expected."
