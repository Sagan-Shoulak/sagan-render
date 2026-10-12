#!/usr/bin/env bash
set -euo pipefail

requested="${SAGAN_RENDER_CMAKE_GENERATOR:-}"
if [[ -n "$requested" ]]; then
  case "$requested" in
    Ninja)
      command -v ninja >/dev/null 2>&1 || {
        echo "error: SAGAN_RENDER_CMAKE_GENERATOR=Ninja requires ninja" >&2
        exit 1
      }
      ;;
    "Unix Makefiles")
      command -v make >/dev/null 2>&1 || {
        echo "error: SAGAN_RENDER_CMAKE_GENERATOR=Unix Makefiles requires make" >&2
        exit 1
      }
      ;;
    *)
      echo "error: unsupported SDL CMake generator: $requested" >&2
      exit 1
      ;;
  esac
  printf '%s\n' "$requested"
elif command -v ninja >/dev/null 2>&1; then
  printf '%s\n' Ninja
elif command -v make >/dev/null 2>&1; then
  printf '%s\n' 'Unix Makefiles'
else
  echo "error: building pinned SDL requires ninja or make" >&2
  exit 1
fi
