#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
mkdir -p build

if [[ -z "${SAGAN_EXECUTABLE:-}" || -z "${SAGAN_PACKAGE_INDEX:-}" ]]; then
  echo "Workspace build requires SAGAN_EXECUTABLE and SAGAN_PACKAGE_INDEX." >&2
  exit 2
fi

"$SAGAN_EXECUTABLE" --emit-cpp-package examples/window_demo build/workspace-window.cpp
