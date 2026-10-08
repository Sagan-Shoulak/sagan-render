#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "Usage: $0 SAGAN_EXECUTABLE NATIVE_EXECUTABLE APPLICATION_NAME APPLICATION_ID" >&2
  exit 2
fi

sagan_executable="$1"
native_executable="$2"
application_name="$3"
application_id="$4"
stage_root="$(dirname "$native_executable")/application"

case "${SAGAN_APPLICATION_PLATFORM:-$(uname -s)}" in
  Linux*)
    icon="$("$sagan_executable" --application-icon linux)"
    icon_name="$(printf '%s' "$application_id" | tr '.' '-')"
    mkdir -p "$stage_root/usr/bin" \
      "$stage_root/usr/share/icons/hicolor/512x512/apps" \
      "$stage_root/usr/share/applications"
    cp "$native_executable" "$stage_root/usr/bin/$icon_name"
    cp "$icon" "$stage_root/usr/share/icons/hicolor/512x512/apps/$icon_name.png"
    printf '%s\n' '[Desktop Entry]' 'Type=Application' "Name=$application_name" \
      "Exec=$icon_name" "Icon=$icon_name" 'Terminal=false' \
      > "$stage_root/usr/share/applications/$icon_name.desktop"
    printf '%s\n' "$stage_root/usr/bin/$icon_name"
    ;;
  Darwin*)
    icon="$("$sagan_executable" --application-icon macos)"
    bundle="$stage_root/$application_name.app"
    mkdir -p "$bundle/Contents/MacOS" "$bundle/Contents/Resources"
    cp "$native_executable" "$bundle/Contents/MacOS/$application_name"
    cp "$icon" "$bundle/Contents/Resources/sagan.icns"
    printf '%s\n' '<?xml version="1.0" encoding="UTF-8"?>' \
      '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' \
      '<plist version="1.0"><dict>' \
      '<key>CFBundlePackageType</key><string>APPL</string>' \
      "<key>CFBundleIdentifier</key><string>$application_id</string>" \
      "<key>CFBundleName</key><string>$application_name</string>" \
      "<key>CFBundleExecutable</key><string>$application_name</string>" \
      '<key>CFBundleIconFile</key><string>sagan.icns</string>' \
      '</dict></plist>' > "$bundle/Contents/Info.plist"
    printf '%s\n' "$bundle/Contents/MacOS/$application_name"
    ;;
  *)
    echo "Application icon staging supports Linux and macOS; Windows embeds the compiler resource at link time." >&2
    exit 1
    ;;
esac
