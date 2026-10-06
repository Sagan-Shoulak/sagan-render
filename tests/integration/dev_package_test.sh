#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

: "${SAGAN_EXECUTABLE:?Set SAGAN_EXECUTABLE to an installed Sagan 4.9.5 executable.}"

output_root="build/dev-package-test/output"
clean_root="${SAGAN_RENDER_CLEAN_ROOT:-$(mktemp -d /tmp/sagan-render-dev-package.XXXXXX)}"
artifact="sagan-render-0.5.1-dev.1-windows-source"
rm -rf "build/dev-package-test" "$clean_root"
mkdir -p "$output_root" "$clean_root"

bash scripts/package-dev.sh "$output_root"
(
  cd "$output_root"
  sha256sum -c "$artifact.zip.sha256"
)
unzip -q "$output_root/$artifact.zip" -d "$clean_root"

package_root="$clean_root/$artifact"
consumer_root="$clean_root/consumer"
grep -Fx "license: GPL-3.0-only" "$package_root/PROVENANCE.txt"
grep -F "GNU GENERAL PUBLIC LICENSE" "$package_root/LICENSE.txt"
grep -F "Zachary Westerman's Schematic" "$package_root/NOTICE.txt"
mkdir -p "$consumer_root"
cp -a examples/window_demo "$consumer_root/window_demo"
cp -a examples/shape_text_demo "$consumer_root/shape_text_demo"

export SAGAN_PACKAGE_INDEX="$package_root/libraries/index.tsv"
export SAGAN_RENDER_AUTOCLOSE_MS=100
(
  cd "$consumer_root"
  "$SAGAN_EXECUTABLE" --run-package window_demo
)

capture="$clean_root/shape-text.bmp"
export SAGAN_RENDER_CAPTURE_BMP="$capture"
export SAGAN_RENDER_AUTOCLOSE_MS=150
(
  cd "$consumer_root"
  "$SAGAN_EXECUTABLE" --run-package shape_text_demo
)

[[ -f "$capture" ]]
[[ "$(head -c 2 "$capture")" == "BM" ]]
dimensions="$(od -An -td4 -j18 -N8 "$capture" | tr -s ' ' | sed 's/^ //;s/ $//')"
[[ "$dimensions" == "960 -540" ]]

if find "$package_root" -type l | grep -q .; then
  echo "Dev package must not contain links to a source checkout." >&2
  exit 1
fi

echo "Dev package test passed: checksum, clean-location package lookup, native bridge, window, and 960x540 capture verified."
