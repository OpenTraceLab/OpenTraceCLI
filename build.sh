#!/bin/bash

# OpenTraceCLI Build Script
# Uses meson and ninja build system

set -e

BUILD_DIR="build"

echo "Building OpenTraceCLI with meson and ninja..."

# Parse command line options
DECODE_OPTION="auto"
SHOW_HELP=false

while [[ $# -gt 0 ]]; do
  case $1 in
    --enable-decode)
      DECODE_OPTION="enabled"
      shift
      ;;
    --disable-decode)
      DECODE_OPTION="disabled"
      shift
      ;;
    --help|-h)
      SHOW_HELP=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      SHOW_HELP=true
      shift
      ;;
  esac
done

if [[ "$SHOW_HELP" == true ]]; then
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  --enable-decode   Force enable protocol decoding support"
  echo "  --disable-decode  Force disable protocol decoding support"
  echo "  --help, -h        Show this help message"
  echo ""
  echo "By default, protocol decoding is auto-detected based on libopentracedecode availability."
  exit 0
fi

# Clean previous build if it exists
if [[ -d "$BUILD_DIR" ]]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Setup build directory with options
echo "Setting up build directory..."
meson setup "$BUILD_DIR" -Ddecode="$DECODE_OPTION"

# Build the project
echo "Building project..."
ninja -C "$BUILD_DIR"

echo "Build complete! Binary is in $BUILD_DIR/opentrace-cli"
echo "To install, run: ninja -C $BUILD_DIR install"
