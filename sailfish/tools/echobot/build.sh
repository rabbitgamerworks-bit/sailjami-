#!/bin/bash
# Build jami-echobot against the libjami built by scripts/build-libjami.sh.
# Run inside the scratchbox2 target, like build-libjami.sh.
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../.." && pwd)
case "$(gcc -dumpmachine)" in
    i?86-*) QT_ARCH=i386 ;;
    armv7*) QT_ARCH=arm ;;
    aarch64-*) QT_ARCH=arm64 ;;
esac
PREFIX=${JAMI_PREFIX:-$ROOT/daemon/install-$QT_ARCH}
OUT=${OUT:-$PREFIX/bin}
mkdir -p "$OUT"
g++ -std=c++20 -O2 -o "$OUT/jami-echobot" "$HERE/echobot.cpp" \
    -I"$PREFIX/include" -L"$PREFIX/lib" -ljami -Wl,-rpath,'$ORIGIN/../lib' -pthread
echo "Built $OUT/jami-echobot"
