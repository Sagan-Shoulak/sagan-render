#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/window-contract

g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  tests/native/window_contract_test.cpp \
  -o build/window-contract/window-contract-test.exe

build/window-contract/window-contract-test.exe
echo "Window contract test passed: dimensions, scale, focus, resize, close, and cleanup state verified."
