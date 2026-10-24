#!/usr/bin/env bash
# Baut den macOS-Installer (.pkg). Aufruf aus dem Repo-Root nach dem CMake-Build.
set -euo pipefail

VERSION="${VERSION:-1.0.0}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VST3="$ROOT/build/NinetyBass_artefacts/Release/VST3/1ninety-bass.vst3"
OUT="$ROOT/dist"
WORK="$ROOT/build/pkgwork"

rm -rf "$WORK"; mkdir -p "$WORK" "$OUT"

# Signieren: Developer ID, falls vorhanden - sonst ad-hoc
if [[ -n "${APP_SIGN_ID:-}" ]]; then
  codesign --force --deep --options runtime --timestamp -s "$APP_SIGN_ID" "$VST3"
else
  codesign --force --deep -s - "$VST3"
fi

pkgbuild --component "$VST3" \
         --install-location "/Library/Audio/Plug-Ins/VST3" \
         --identifier "com.taubenopa.ninetybass.vst3" \
         --version "$VERSION" \
         "$WORK/component.pkg"

PB_ARGS=(--distribution "$ROOT/installer/macos/distribution.xml"
         --resources "$ROOT/installer/macos"
         --package-path "$WORK")
[[ -n "${PKG_SIGN_ID:-}" ]] && PB_ARGS+=(--sign "$PKG_SIGN_ID" --timestamp)

productbuild "${PB_ARGS[@]}" "$OUT/1ninety-bass-$VERSION-macOS.pkg"
echo "Fertig: $OUT/1ninety-bass-$VERSION-macOS.pkg"
