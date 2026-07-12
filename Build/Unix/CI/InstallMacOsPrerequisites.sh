#!/bin/bash
# CI helper script to install MacOS packages, MethaneKit build prerequisites.

version_ge() {
    [ "$(printf '%s\n' "$1" "$2" | sort -V | head -n1)" != "$1" ] || [ "$1" = "$2" ]
}

# Minimum XCode version since which xcodebuild has command -downloadComponent
XCODE_CURRENT_VERSION=$(xcodebuild -version | grep "Xcode" | awk '{print $2}')
echo "XCode version: $XCODE_CURRENT_VERSION"

if version_ge "$XCODE_CURRENT_VERSION" "26.0"; then
    echo "Downloading Metal toolchain..."
    xcodebuild -downloadComponent MetalToolchain
    xcrun --find metal
    xcrun metal -v
fi