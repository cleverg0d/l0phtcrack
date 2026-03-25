#!/usr/bin/env bash
# Build (if needed) and zip lc7.app for local use. Run from any directory.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${1:-$REPO_ROOT/build-macos}"
OUT_ZIP="${2:-$BUILD_DIR/lc7-macos-$(uname -m).zip}"

if [[ ! -d "$BUILD_DIR" ]]; then
	echo "Build directory not found: $BUILD_DIR"
	echo "Usage: $0 [build-dir] [output.zip]"
	exit 1
fi

echo "Configure and build (lc7 target pulls Qt into the bundle)..."
cmake -S "$REPO_ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target lc7 -j "$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

APP="$BUILD_DIR/dist/lc7.app"
if [[ ! -d "$APP" ]]; then
	echo "Missing: $APP"
	exit 1
fi

IMPORT_MANIFEST="$APP/Contents/MacOS/lcplugins/lc7importwin{17324176-3fa7-4c1a-9204-3f391b6b3599}/manifest.json"
if [[ ! -f "$IMPORT_MANIFEST" ]]; then
	echo "ERROR: lc7importwin manifest missing in bundle (macOS build is broken):"
	echo "  $IMPORT_MANIFEST"
	echo "Ensure lc7/CMakeLists.txt copies manifest after macdeployqt, then rebuild lc7."
	exit 1
fi

echo "Ad-hoc sign (silences some local gatekeeper checks)..."
codesign --force --deep -s - "$APP" 2>/dev/null || {
	echo "codesign failed (install Xcode CLI tools). Zip is still created."
}

( cd "$BUILD_DIR/dist" && rm -f "$(basename "$OUT_ZIP")" && zip -r -q "$OUT_ZIP" lc7.app )
echo "Done: $OUT_ZIP"
echo ""
echo "If the app does not open when copied from the internet or another Mac:"
echo "  xattr -cr \"$APP\""
echo "Or after unzip:"
echo "  xattr -cr ~/Downloads/lc7.app"
echo ""
echo "Smoke test from Terminal (shows errors in this window):"
echo "  \"$APP/Contents/MacOS/lc7\""
