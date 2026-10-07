#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if [[ "$(uname -s)" != "Linux" || "$(uname -m)" != "x86_64" ]]; then
  echo "Material shader generation requires the pinned Linux x86-64 toolchain." >&2
  exit 1
fi

toolchain_report=build/shader-toolchain/toolchain-report.txt
if [[ ! -f "$toolchain_report" ]]; then
  echo "Build the pinned shader toolchain first: bash scripts/build-shader-toolchain-linux.sh" >&2
  exit 1
fi

report_value() {
  local key="$1"
  local value
  value="$(sed -n "s/^${key}=\(.*\)$/\1/p" "$toolchain_report")"
  if [[ -z "$value" ]]; then
    echo "Missing toolchain report value: $key" >&2
    exit 1
  fi
  printf '%s\n' "$value"
}

shadercross_cli="$(report_value cli)"
dxc_library_path="$(report_value dxc_library_path)"
[[ -x "$shadercross_cli" ]]
[[ -d "$dxc_library_path" ]]

source_file=shaders/material/lit_mesh.hlsl
manifest_file=shaders/material/lit_mesh.shader
output_root="${1:-build/generated-shaders/material/lit_mesh}"
mkdir -p "$output_root"

compile_stage() {
  local stage="$1"
  local entrypoint="$2"
  local stem="$3"
  local format extension

  for format in DXIL SPIRV MSL JSON; do
    case "$format" in
      DXIL) extension=dxil ;;
      SPIRV) extension=spv ;;
      MSL) extension=msl ;;
      JSON) extension=json ;;
    esac
    LD_LIBRARY_PATH="$dxc_library_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
      "$shadercross_cli" "$source_file" \
      --source HLSL \
      --dest "$format" \
      --stage "$stage" \
      --entrypoint "$entrypoint" \
      --output "$output_root/lit_mesh.$stem.$extension"
  done
}

compile_stage vertex VSMain vert
compile_stage fragment PSMain frag

source_sha256="$(sha256sum "$source_file" | cut -d ' ' -f1)"
manifest_sha256="$(sha256sum "$manifest_file" | cut -d ' ' -f1)"
{
  echo "source=$source_file"
  echo "source_sha256=$source_sha256"
  echo "manifest=$manifest_file"
  echo "manifest_sha256=$manifest_sha256"
  grep -E '^(sdl_commit|shadercross_commit|spirv_cross_commit|dxc_release|dxc_sha256)=' "$toolchain_report"
  echo "vertex_entrypoint=VSMain"
  echo "fragment_entrypoint=PSMain"
  for artifact in "$output_root"/lit_mesh.*; do
    if [[ "$artifact" == "$output_root/lit_mesh.artifacts.sha256" ]]; then
      continue
    fi
    digest="$(sha256sum "$artifact" | cut -d ' ' -f1)"
    printf '%s  %s\n' "$digest" "$(basename "$artifact")"
  done
} > "$output_root/lit_mesh.artifacts.sha256"

echo "Generated material shader artifacts: $output_root"
