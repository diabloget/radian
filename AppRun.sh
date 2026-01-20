#!/bin/bash
HERE=$(dirname "$(readlink -f "$0")")
BIN="$HERE/usr/bin/radian"

# Configure library path to use bundled libraries
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"

# Detect the path of the mounted AppImage
APPIMAGE_PATH="${APPIMAGE:-$0}"

# Check if we are already root
if [ "$EUID" -ne 0 ]; then
    # Copy binary to /tmp to avoid noexec problems
    TMP_BIN="/tmp/radian-$$"
    cp "$BIN" "$TMP_BIN"
    chmod +x "$TMP_BIN"
    
    # Copy bundled libraries to /tmp
    TMP_LIB="/tmp/radian-lib-$$"
    mkdir -p "$TMP_LIB"
    cp -r "$HERE/usr/lib/"* "$TMP_LIB/" 2>/dev/null
    
    if command -v pkexec >/dev/null 2>&1; then
        # Pass environment variables through pkexec
        pkexec env DISPLAY="$DISPLAY" \
                   XAUTHORITY="$XAUTHORITY" \
                   APPIMAGE="$APPIMAGE_PATH" \
                   LD_LIBRARY_PATH="$TMP_LIB:$LD_LIBRARY_PATH" \
                   "$TMP_BIN"
        RET=$?
    else
        # Fallback: terminal with sudo
        x-terminal-emulator -e bash -c "APPIMAGE='$APPIMAGE_PATH' LD_LIBRARY_PATH='$TMP_LIB' sudo '$TMP_BIN'" || \
        xterm -e bash -c "APPIMAGE='$APPIMAGE_PATH' LD_LIBRARY_PATH='$TMP_LIB' sudo '$TMP_BIN'"
        RET=$?
    fi
    
    # Clean temporary files
    rm -f "$TMP_BIN"
    rm -rf "$TMP_LIB"
    exit $RET
fi

# If we are already root, run directly
export APPIMAGE="$APPIMAGE_PATH"
exec "$BIN" "$@"