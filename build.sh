#!/usr/bin/env bash
# Drift — Ocean Engine build script
# Usage: ./build.sh [run]

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
APP_DIR="$BUILD_DIR/Drift.app"
CONTENTS="$APP_DIR/Contents"
MACOS="$CONTENTS/MacOS"

DSP_DIR="$PROJECT_DIR/Sources/DSP"
SWIFT_DIR="$PROJECT_DIR/Sources"
OBJ_DIR="$BUILD_DIR/obj"

SDK="$(xcrun --show-sdk-path --sdk macosx)"
SWIFT_LIBS="$(dirname "$(xcrun --find swiftc)")/../lib/swift/macosx"

mkdir -p "$MACOS" "$OBJ_DIR"

echo "▶  Compiling C++ DSP core..."

CXXFLAGS="-std=c++17 -O2 -ffast-math -fno-exceptions -fno-rtti \
    -arch arm64 -mmacosx-version-min=13.0 \
    -I$DSP_DIR"

clang++ $CXXFLAGS \
    -c "$DSP_DIR/OceanDSP.cpp" \
    -o "$OBJ_DIR/OceanDSP.o"

clang++ $CXXFLAGS -ObjC++ \
    -c "$DSP_DIR/OceanBridge.mm" \
    -o "$OBJ_DIR/OceanBridge.o"

# Static lib so swiftc can link against it
ar rcs "$OBJ_DIR/libOceanDSP.a" "$OBJ_DIR/OceanDSP.o" "$OBJ_DIR/OceanBridge.o"

echo "▶  Compiling + linking Swift app layer..."

swiftc \
    -O \
    -sdk "$SDK" \
    -target arm64-apple-macos13.0 \
    -module-name Drift \
    -import-objc-header "$DSP_DIR/OceanBridge.h" \
    "$SWIFT_DIR/main.swift" \
    "$SWIFT_DIR/AppDelegate.swift" \
    "$SWIFT_DIR/OceanEngine.swift" \
    -L"$OBJ_DIR" -lOceanDSP \
    -lc++ \
    -framework Cocoa \
    -framework AVFoundation \
    -framework AudioToolbox \
    -Xlinker -rpath -Xlinker /usr/lib/swift \
    -o "$MACOS/Drift"

cp "$PROJECT_DIR/Info.plist" "$CONTENTS/Info.plist"

echo "✅  Built: $APP_DIR"

if [[ "${1:-}" == "run" ]]; then
    echo "▶  Launching..."
    open "$APP_DIR"
fi
