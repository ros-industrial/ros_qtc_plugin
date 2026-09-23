#!/usr/bin/env bash

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <path-to-lzma-deb-file>"
    exit 1
fi

INPUT_DEB="$(realpath "$1")"
WORK_DIR=$(mktemp -d -t deb-repack-XXXXXX)

echo "📦 Creating temporary workspace at: $WORK_DIR"
cd "$WORK_DIR"

echo "📂 Extracting original deb components..."
ar x "$INPUT_DEB"

if [ ! -f "control.tar.lzm" ]; then
    echo "❌ Error: This package does not seem to contain a control.tar.lzm."
    rm -rf "$WORK_DIR"
    exit 1
fi

echo "Recompressing LZM to XZ..."
xz --decompress --stdout control.tar.lzm | xz > control.tar.xz
xz --decompress --stdout data.tar.lzma | xz > data.tar.xz


# override original Debian file
OUTPUT_DEB="$INPUT_DEB"

echo "🔨 Building modern Debian archive..."
rm -f "$OUTPUT_DEB"
ar rcs "$OUTPUT_DEB" debian-binary control.tar.xz data.tar.xz

echo "🧹 Cleaning up workspace..."
rm -rf "$WORK_DIR"
