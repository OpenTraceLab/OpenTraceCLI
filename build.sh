#!/bin/bash

# OpenTraceCLI Build Script
# Uses meson and ninja build system

set -e

BUILD_DIR="build"

echo "Building OpenTraceCLI with meson and ninja..."

# Clean previous build if it exists
if [[ -d "$BUILD_DIR" ]]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Setup build directory
echo "Setting up build directory..."
meson setup "$BUILD_DIR"

# Build the project
echo "Building project..."
ninja -C "$BUILD_DIR"

echo "Build complete! Binary is in $BUILD_DIR/opentrace-cli"
echo "To install, run: ninja -C $BUILD_DIR install"
