#!/usr/bin/env bash
set -euo pipefail

export PATH="/c/msys64/ucrt64/bin:/ucrt64/bin:/usr/bin:/bin:$PATH"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
pin="$(tr -d '\r\n' < "$repo_root/sagan-source-commit.txt")"
short_pin="${pin:0:8}"
cd "$repo_root"
mkdir -p build/tmp
fixture_root="$(mktemp -d build/tmp/compiler-resolver.XXXXXX)"
trap 'rm -rf "$fixture_root"' EXIT

executable_name="sagan"
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) executable_name="sagan.exe" ;;
esac

mkdir -p "$fixture_root/stale-bin" \
         "$fixture_root/cache/sagan-pinned-$short_pin/bin" \
         "$fixture_root/explicit"
printf '#!/usr/bin/env bash\nexit 91\n' > "$fixture_root/stale-bin/sagan"
printf '#!/usr/bin/env bash\nexit 0\n' > \
  "$fixture_root/cache/sagan-pinned-$short_pin/bin/$executable_name"
printf '#!/usr/bin/env bash\nexit 0\n' > "$fixture_root/explicit/$executable_name"
chmod +x "$fixture_root/stale-bin/sagan" \
  "$fixture_root/cache/sagan-pinned-$short_pin/bin/$executable_name" \
  "$fixture_root/explicit/$executable_name"

resolved="$(
  PATH="$fixture_root/stale-bin:$PATH" \
  SAGAN_COMPILER_CACHE_ROOT="$fixture_root/cache" \
  bash "$repo_root/scripts/resolve-sagan.sh"
)"
expected="$fixture_root/cache/sagan-pinned-$short_pin/bin/$executable_name"
[[ "$resolved" == "$expected" ]]

resolved="$(
  SAGAN_EXECUTABLE="$fixture_root/explicit/$executable_name" \
  bash "$repo_root/scripts/resolve-sagan.sh"
)"
[[ "$resolved" == "$fixture_root/explicit/$executable_name" ]]

printf 'not-a-commit\n' > "$fixture_root/bad-pin.txt"
if SAGAN_SOURCE_PIN_FILE="$fixture_root/bad-pin.txt" \
   bash "$repo_root/scripts/resolve-sagan.sh" >/dev/null 2>&1; then
  echo "Malformed compiler source pin was accepted" >&2
  exit 1
fi

echo "Compiler resolver test passed: explicit override, exact cache, stale PATH rejection, and pin validation verified."
