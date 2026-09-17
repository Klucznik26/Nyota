#!/bin/bash
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NYOTA="$ROOT/nyota"
TDIR="$ROOT/tests"
fail=0
pass=0
skip=0

if [ ! -x "$NYOTA" ]; then
    echo "brak ./nyota — najpierw: make"
    exit 1
fi

for f in "$TDIR"/*.nyo; do
    base=$(basename "$f")
    expect=$(grep -m1 '^# Oczekiwane:' "$f" | cut -d: -f2- | sed 's/^[[:space:]]*//')
    if [ -z "$expect" ]; then
        echo "SKIP $base (brak Oczekiwane)"
        skip=$((skip + 1))
        continue
    fi
    out=$("$NYOTA" "$f" 2>&1) || true
    case "$expect" in
    BLAD*)
        if echo "$out" | grep -q BLAD; then
            echo "PASS $base"
            pass=$((pass + 1))
        else
            echo "FAIL $base — oczekiwano bledu"
            echo "$out" | sed 's/^/  /'
            fail=$((fail + 1))
        fi
        ;;
    *)
        if echo "$out" | grep -q BLAD; then
            echo "FAIL $base — nieoczekiwany BLAD"
            echo "$out" | sed 's/^/  /'
            fail=$((fail + 1))
        else
            echo "PASS $base"
            pass=$((pass + 1))
        fi
        ;;
    esac
done

echo "---"
echo "pass=$pass fail=$fail skip=$skip"
test "$fail" -eq 0
