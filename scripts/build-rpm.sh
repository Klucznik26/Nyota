#!/bin/bash
# Buduje lokalny pakiet RPM Nyoty na Fedorze z bieżącego drzewa.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SPEC="$ROOT/packaging/nyota.spec"
NAME="$(awk '$1 == "Name:" { print $2; exit }' "$SPEC")"
VER="$(awk '$1 == "Version:" { print $2; exit }' "$SPEC")"

# rpmbuild nie lubi nietypowych ścieżek roboczych; trzymamy topdir w /tmp.
TOP="${NYOTA_RPM_TOPDIR:-/tmp/nyota-rpmbuild}"
TARDIR="$NAME-$VER"

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Brak wymaganego narzędzia: $1" >&2
        echo "Na Fedorze zainstaluj:" >&2
        echo "  sudo dnf install rpm-build gcc make pkgconf-pkg-config sdl2-compat-devel tar gzip" >&2
        exit 2
    fi
}

for cmd in rpmbuild gcc make pkg-config tar gzip awk find; do
    need_cmd "$cmd"
done

if ! pkg-config --exists sdl2; then
    echo "Brak pkgconfig(sdl2)." >&2
    echo "Na Fedorze 44 zapewnia go pakiet sdl2-compat-devel:" >&2
    echo "  sudo dnf install sdl2-compat-devel" >&2
    exit 2
fi

rm -rf "$TOP"
mkdir -p "$TOP"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/$TARDIR"

# Źródła bez artefaktów z poprzednich buildów i bez repozytorium Git.
tar -C "$ROOT" \
    --exclude=nyota \
    --exclude=.git \
    --exclude='packaging/*.rpm' \
    --exclude='packaging/*.src.rpm' \
    --exclude=packaging/rpmbuild \
    -cf - . | tar -C "$TMP/$TARDIR" -xf -

tar -C "$TMP" -czf "$TOP/SOURCES/$NAME-$VER.tar.gz" "$TARDIR"
cp "$SPEC" "$TOP/SPECS/"

rpmbuild \
    --define "_topdir $TOP" \
    -ba "$TOP/SPECS/nyota.spec"

OUT="$ROOT/packaging"
mkdir -p "$OUT"
find "$TOP/RPMS" "$TOP/SRPMS" -name '*.rpm' -exec cp -v {} "$OUT/" \;

echo
echo "Gotowe pakiety RPM:"
find "$OUT" -maxdepth 1 -type f \( -name "$NAME-$VER-*.rpm" -o -name "$NAME-$VER-*.src.rpm" \) -print
