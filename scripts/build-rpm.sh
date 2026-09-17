#!/bin/bash
# Buduje RPM Nyoty na Fedorze z bieżącego drzewa.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VER=0.5.0
NAME=nyota
# rpmbuild psuje ścieżki ze spacją (AYO Projekty).
TOP="${NYOTA_RPM_TOPDIR:-/tmp/nyota-rpmbuild}"
TARDIR="$NAME-$VER"

rm -rf "$TOP"
mkdir -p "$TOP"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/$TARDIR"
# Źródła bez binarki i bez rpmbuild
tar -C "$ROOT" --exclude=nyota --exclude=packaging/rpmbuild --exclude=.git \
    -cf - . | tar -C "$TMP/$TARDIR" -xf -
tar -C "$TMP" -czf "$TOP/SOURCES/$NAME-$VER.tar.gz" "$TARDIR"
cp "$ROOT/packaging/nyota.spec" "$TOP/SPECS/"

rpmbuild \
    --define "_topdir $TOP" \
    -ba "$TOP/SPECS/nyota.spec"

OUT="$ROOT/packaging"
mkdir -p "$OUT"
find "$TOP/RPMS" "$TOP/SRPMS" -name '*.rpm' -exec cp -v {} "$OUT/" \;
echo
echo "RPM skopiowane do $OUT :"
ls -lh "$OUT"/*.rpm
