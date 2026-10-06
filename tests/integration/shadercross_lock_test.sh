#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
lock=third_party/sdl-shadercross.lock

grep -q '^source_commit = "[0-9a-f]\{40\}"$' "$lock"
grep -q '^spirv_cross_commit = "[0-9a-f]\{40\}"$' "$lock"
grep -q '^spirv_headers_commit = "[0-9a-f]\{40\}"$' "$lock"
grep -q '^spirv_tools_commit = "[0-9a-f]\{40\}"$' "$lock"
grep -q '^directx_shader_compiler_commit = "[0-9a-f]\{40\}"$' "$lock"
grep -q '^dxc_spirv_headers_commit = "[0-9a-f]\{40\}"$' "$lock"
grep -q '^dxc_spirv_tools_commit = "[0-9a-f]\{40\}"$' "$lock"
grep -q '^dxc_directx_headers_commit = "[0-9a-f]\{40\}"$' "$lock"
grep -q '^dxc_googletest_gitlink = "absent-at-pinned-commit"$' "$lock"
grep -q '^submodule_graph_verified = true$' "$lock"
grep -q -- '--no-recurse-submodules' scripts/fetch-shadercross-source.sh

lock_source="$(grep '^source_commit = ' "$lock" | cut -d '"' -f2)"
script_source="$(grep '^source_commit=' scripts/fetch-shadercross-source.sh | cut -d '"' -f2)"
[[ "$lock_source" == "$script_source" ]]

echo "SDL_shadercross lock passed: top-level and nested DXC gitlinks are explicit; source fetch remains non-recursive."
