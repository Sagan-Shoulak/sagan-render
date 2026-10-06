#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

version="0.5.1"
channel="dev.1"
artifact="sagan-render-${version}-${channel}-windows-source"
output_root="${1:-build/dev-package}"
stage="$output_root/$artifact"
archive="$output_root/$artifact.zip"
checksum="$archive.sha256"

if ! git diff --quiet || ! git diff --cached --quiet; then
  echo "Dev packages must be produced from a clean reviewed commit." >&2
  exit 2
fi

commit="$(git rev-parse HEAD)"
rm -rf "$stage" "$archive" "$checksum"
mkdir -p "$stage/libraries/render"

cp libraries/index.tsv "$stage/libraries/index.tsv"
cp libraries/render/sagan.toml "$stage/libraries/render/sagan.toml"
cp -a libraries/render/src "$stage/libraries/render/src"
cp -a libraries/render/native "$stage/libraries/render/native"
cp LICENSE.txt NOTICE.txt "$stage/"

{
  echo "sagan-render Windows dev-channel package"
  echo "package-version: $version"
  echo "channel: $channel"
  echo "source-repository: https://github.com/Sagan-Shoulak/sagan-render"
  echo "source-commit: $commit"
  echo "compiler-compatibility: ^4.0.0"
  echo "supported-platform: Windows"
  echo "license: GPL-3.0-only"
  echo "license-file: LICENSE.txt"
  echo "notices-file: NOTICE.txt"
  echo "package-index: libraries/index.tsv"
  echo "native-bridge: libraries/render/native"
} > "$stage/PROVENANCE.txt"

(
  cd "$output_root"
  /c/Windows/System32/tar.exe -a -c -f "$(basename "$archive")" "$artifact"
)
(
  cd "$output_root"
  sha256sum "$(basename "$archive")" > "$(basename "$checksum")"
)

echo "$archive"
echo "$checksum"
