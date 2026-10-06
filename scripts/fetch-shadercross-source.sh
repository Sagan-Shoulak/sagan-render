#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

source_repository="https://github.com/libsdl-org/SDL_shadercross.git"
source_commit="1ff05bec573988a98ef9e0260b4da44f512b8367"
short_commit="${source_commit:0:8}"
source_directory="build/dependencies/SDL_shadercross-$short_commit"

if [[ -d "$source_directory/.git" ]]; then
  actual_commit="$(git -C "$source_directory" rev-parse HEAD)"
  if [[ "$actual_commit" != "$source_commit" ]]; then
    echo "SDL_shadercross cache mismatch: expected $source_commit, got $actual_commit" >&2
    exit 1
  fi
  printf '%s\n' "$source_directory"
  exit 0
fi

if [[ -e "$source_directory" ]]; then
  echo "SDL_shadercross cache exists but is not a Git checkout: $source_directory" >&2
  exit 1
fi

mkdir -p build/dependencies
echo "Fetching SDL_shadercross source $short_commit without submodules..." >&2
git clone --filter=blob:none --no-checkout "$source_repository" "$source_directory" >&2
git -C "$source_directory" fetch origin "$source_commit" >&2
git -C "$source_directory" checkout --detach "$source_commit" >&2

actual_commit="$(git -C "$source_directory" rev-parse HEAD)"
[[ "$actual_commit" == "$source_commit" ]]
[[ -f "$source_directory/LICENSE.txt" ]]
[[ -f "$source_directory/CMakeLists.txt" ]]
printf '%s\n' "$source_directory"
