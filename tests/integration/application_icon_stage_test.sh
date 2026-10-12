#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
fixture=build/application-icon-stage-test
rm -rf "$fixture"
mkdir -p "$fixture/compiler" "$fixture/native"

printf 'canonical-sagan-icns\n' > "$fixture/compiler/sagan.icns"
cat > "$fixture/compiler/sagan" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
[[ "${1:-}" == --application-icon && "${2:-}" == macos ]]
printf '%s\n' "$SAGAN_ICON_FIXTURE"
EOF
cat > "$fixture/native/test-host" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'cwd=%s\n' "$PWD" > "$SAGAN_LAUNCH_REPORT"
printf 'environment=%s\n' "$SAGAN_LAUNCH_VALUE" >> "$SAGAN_LAUNCH_REPORT"
printf 'argc=%s\n' "$#" >> "$SAGAN_LAUNCH_REPORT"
printf 'arg1=%s\n' "$1" >> "$SAGAN_LAUNCH_REPORT"
printf 'arg2=%s\n' "$2" >> "$SAGAN_LAUNCH_REPORT"
exit 23
EOF
chmod +x "$fixture/compiler/sagan" "$fixture/native/test-host"

export SAGAN_ICON_FIXTURE="$repo_root/$fixture/compiler/sagan.icns"
staged="$(SAGAN_APPLICATION_PLATFORM=Darwin bash scripts/stage-application-icon.sh \
  "$fixture/compiler/sagan" "$fixture/native/test-host" \
  "Sagan Test Host" "org.saganshoulak.render.test-host")"
bundle="$fixture/native/application/Sagan Test Host.app"
[[ "$staged" == "$bundle/Contents/MacOS/Sagan Test Host" ]]
cmp "$fixture/compiler/sagan.icns" "$bundle/Contents/Resources/sagan.icns"
grep -q '<key>CFBundleIdentifier</key><string>org.saganshoulak.render.test-host</string>' \
  "$bundle/Contents/Info.plist"
grep -q '<key>CFBundleIconFile</key><string>sagan.icns</string>' \
  "$bundle/Contents/Info.plist"
grep -q '<key>CFBundleExecutable</key><string>Sagan Test Host</string>' \
  "$bundle/Contents/Info.plist"

export SAGAN_LAUNCH_REPORT="$repo_root/$fixture/launch-report.txt"
export SAGAN_LAUNCH_VALUE=preserved
set +e
"$staged" 'first argument' second
status=$?
set -e
[[ $status -eq 23 ]]
grep -q "^cwd=$repo_root$" "$fixture/launch-report.txt"
grep -q '^environment=preserved$' "$fixture/launch-report.txt"
grep -q '^argc=2$' "$fixture/launch-report.txt"
grep -q '^arg1=first argument$' "$fixture/launch-report.txt"
grep -q '^arg2=second$' "$fixture/launch-report.txt"

for launcher in scene surface loading toolbar ui_gpu lighting; do
  grep -q 'stage-application-icon.sh' "scripts/${launcher}_demo_portable.sh"
done

echo "macOS application staging passed: canonical ICNS, bundle metadata, launcher coverage, arguments, environment, cwd, and exit status verified."
