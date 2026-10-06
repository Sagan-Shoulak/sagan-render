#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

commit="636f4ff82cbf1bba7de3836023a752610336fe77"
base_url="https://raw.githubusercontent.com/TheSpydog/SDL_gpu_examples/$commit/Content/Shaders/Compiled"
destination="build/dependencies/sdl-gpu-foundation-shaders"
mkdir -p "$destination"

fetch() {
  local relative="$1"
  local output="$2"
  local expected="$3"
  if [[ ! -f "$destination/$output" ]]; then
    curl --fail --silent --show-error --location --output "$destination/$output" "$base_url/$relative"
  fi
  local actual
  if command -v sha256sum >/dev/null 2>&1; then
    actual="$(sha256sum "$destination/$output" | cut -d ' ' -f1)"
  else
    actual="$(shasum -a 256 "$destination/$output" | cut -d ' ' -f1)"
  fi
  if [[ "$actual" != "$expected" ]]; then
    echo "Shader checksum mismatch for $output: expected $expected, got $actual" >&2
    exit 1
  fi
}

fetch DXIL/RawTriangle.vert.dxil RawTriangle.vert.dxil de6c95c47b8f70687517e5c98d556197dd27365a71d835416663b23e0e330950
fetch DXIL/SolidColor.frag.dxil SolidColor.frag.dxil c83e610513d13aa98ba823d099efbeb8576c062f1917ec56430894c74366cfc7
fetch SPIRV/RawTriangle.vert.spv RawTriangle.vert.spv f8a62c2d8dcd2c6e1974b2282582b8b985c63e0efafe1656971481d421b8bc61
fetch SPIRV/SolidColor.frag.spv SolidColor.frag.spv 15faae269701535407becf129b58b204067c421ad96d8ee2ea0403d65fd19959
fetch MSL/RawTriangle.vert.msl RawTriangle.vert.msl 485ae6e59a89345abd3d0f548b9b75cc889f2cc3944794ee8d30b1e284d063a0
fetch MSL/SolidColor.frag.msl SolidColor.frag.msl d86fccd67bec4d3943e88c2f666b73ee33a156a0d8560cdb0833fef7279e215e

if command -v sha256sum >/dev/null 2>&1; then
  source_digest="$(sha256sum shaders/foundation/RawTriangle.vert.hlsl shaders/foundation/SolidColor.frag.hlsl)"
else
  source_digest="$(shasum -a 256 shaders/foundation/RawTriangle.vert.hlsl shaders/foundation/SolidColor.frag.hlsl)"
fi
grep -q '^e66081ebec3b75734cb04a03aafe167825dd24ba42d4dc79d478b45c9f21ae65 ' <<<"$source_digest"
grep -q '^12ece7e3b7adade6adba5cebad25380240d8173d541e2241b37dab1c903a0993 ' <<<"$source_digest"

echo "$repo_root/$destination"
