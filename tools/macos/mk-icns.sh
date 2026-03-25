#!/bin/bash
# Regenerate lc7/resources/lc7.icns from lc7/resources/lc7icon_512.png (macOS: sips + iconutil).
# iconutil may fail under restricted sandboxes; run on a normal macOS shell if needed.
set -eu
SRC_ROOT="$1"
OUT_ICNS="${2:-${SRC_ROOT}/lc7/resources/lc7.icns}"
MASTER="${SRC_ROOT}/lc7/resources/lc7icon_512.png"
if [[ ! -f "$MASTER" ]]; then
	echo "mk-icns.sh: missing $MASTER" >&2
	exit 1
fi
ICONSET="$(mktemp -d "${TMPDIR:-/tmp}/lc7icon.XXXXXX").iconset"
cleanup() { rm -rf "$ICONSET"; }
trap cleanup EXIT
mkdir "$ICONSET"

sips -z 16 16 "$MASTER" --out "$ICONSET/icon_16x16.png" >/dev/null
sips -z 32 32 "$MASTER" --out "$ICONSET/icon_16x16@2x.png" >/dev/null
sips -z 32 32 "$MASTER" --out "$ICONSET/icon_32x32.png" >/dev/null
sips -z 64 64 "$MASTER" --out "$ICONSET/icon_32x32@2x.png" >/dev/null
sips -z 128 128 "$MASTER" --out "$ICONSET/icon_128x128.png" >/dev/null
sips -z 256 256 "$MASTER" --out "$ICONSET/icon_128x128@2x.png" >/dev/null
sips -z 256 256 "$MASTER" --out "$ICONSET/icon_256x256.png" >/dev/null
sips -z 512 512 "$MASTER" --out "$ICONSET/icon_256x256@2x.png" >/dev/null
sips -z 512 512 "$MASTER" --out "$ICONSET/icon_512x512.png" >/dev/null
sips -z 1024 1024 "$MASTER" --out "$ICONSET/icon_512x512@2x.png" >/dev/null

mkdir -p "$(dirname "$OUT_ICNS")"
iconutil -c icns "$ICONSET" -o "$OUT_ICNS"
echo "Wrote $OUT_ICNS"
