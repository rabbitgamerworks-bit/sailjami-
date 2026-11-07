#!/bin/bash
# Build libjami (jami-daemon) and its contrib dependencies for Sailfish OS.
#
# Run inside the Sailfish SDK build engine, in the scratchbox2 target, e.g.:
#   sfdk engine exec sb2 -t SailfishOS-5.1.0.11-i486 bash sailfish/scripts/build-libjami.sh
#
# Contrib tarballs must be present in daemon/contrib/tarballs (run
# "make fetch" from a bootstrapped contrib build dir on a machine with
# internet access), because the build engine may not have direct access.
#
# Result: daemon/install-<qt arch>/{include,lib}, used by sailjami.pro.
#
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
DAEMON=$ROOT/daemon
TRIPLET=$(gcc -dumpmachine)
case "$TRIPLET" in
    i486-*|i586-*|i686-*) QT_ARCH=i386 ;;
    armv7*) QT_ARCH=arm ;;
    aarch64-*) QT_ARCH=arm64 ;;
    *) echo "Unsupported target $TRIPLET" >&2; exit 1 ;;
esac
PREFIX=${JAMI_PREFIX:-$DAEMON/install-$QT_ARCH}
JOBS=${JOBS:-$(nproc)}

echo "== Building contrib for $TRIPLET"
mkdir -p "$DAEMON/contrib/build-$TRIPLET"
cd "$DAEMON/contrib/build-$TRIPLET"
# Video codecs are not needed for the text-only first iteration.
../bootstrap --host="$TRIPLET" --disable-downloads \
    --disable-vpx --disable-x264 --disable-ffnvcodec
make -j"$JOBS"

echo "== Building libjami for $TRIPLET"
mkdir -p "$DAEMON/build-$TRIPLET"
cd "$DAEMON/build-$TRIPLET"
cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DBUILD_CONTRIB=OFF \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=OFF \
    -DJAMI_VIDEO=OFF \
    -DJAMI_VIDEO_ACCEL=OFF \
    -DJAMI_PLUGIN=OFF \
    -DJAMI_DBUS=OFF
ninja -j"$JOBS"
ninja install
echo "== libjami installed to $PREFIX"
