#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
mkdir -p build/ui-contract

executable="build/ui-contract/ui-contract-test"
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) executable="${executable}.exe" ;;
esac

"${CXX:-c++}" -std=c++23 -Wall -Wextra -Wpedantic -Werror \
  tests/native/ui_contract_test.cpp -o "$executable"
"$executable" build/ui-contract/ui-layout.svg

grep -q 'data-contract="sagan-ui-logical-v1"' build/ui-contract/ui-layout.svg
grep -q 'viewBox="0 0 800 450"' build/ui-contract/ui-layout.svg
echo "UI contract test passed: logical layout, physical view span, display scale, shaping boundary, input routing, and cleanup verified."
