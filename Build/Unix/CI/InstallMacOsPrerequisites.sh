#!/bin/bash
# CI helper script to install MacOS packages, MethaneKit build prerequisites.

# Numeric version comparison, returns success when $1 >= $2.
# Implemented with awk instead of 'sort -V', which is a GNU extension not available in BSD sort on macOS:
# without it the comparison degrades to lexicographic order, where "9.0" would rank above "10.0".
version_ge() {
    awk -v v1="$1" -v v2="$2" 'BEGIN {
        n1 = split(v1, a1, "."); n2 = split(v2, a2, ".")
        n = (n1 > n2) ? n1 : n2
        for (i = 1; i <= n; i++) {
            p1 = (i <= n1) ? a1[i] + 0 : 0
            p2 = (i <= n2) ? a2[i] + 0 : 0
            if (p1 != p2) exit (p1 > p2) ? 0 : 1
        }
        exit 0
    }'
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