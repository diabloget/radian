#!/bin/bash
HERE=$(dirname "$(readlink -f "$0")")
BIN="$HERE/usr/bin/radian"

# Detects the path of the mounted AppImage
# $APPIMAGE contains the full path to the .AppImage original
APPIMAGE_PATH="${APPIMAGE:-$0}"

# Checks if we are already root
if [ "$EUID" -ne 0 ]; then
    # Copies the binary to /tmp to avoid noexec problems
    TMP_BIN="/tmp/radian-$$"
    cp "$BIN" "$TMP_BIN"
    chmod +x "$TMP_BIN"
    
    if command -v pkexec >/dev/null 2>&1; then
        # Pass APPIMAGE explicitly through pkexec
        pkexec env DISPLAY="$DISPLAY" \
                   XAUTHORITY="$XAUTHORITY" \
                   APPIMAGE="$APPIMAGE_PATH" \
                   "$TMP_BIN"
        RET=$?
    else
        # Fallback: terminal with sudo
        x-terminal-emulator -e bash -c "APPIMAGE='$APPIMAGE_PATH' sudo '$TMP_BIN'" || \
        xterm -e bash -c "APPIMAGE='$APPIMAGE_PATH' sudo '$TMP_BIN'"
        RET=$?
    fi
    
    # Clean temporary file
    rm -f "$TMP_BIN"
    exit $RET
fi

# If we are already root, run directly with APPIMAGE configured
export APPIMAGE="$APPIMAGE_PATH"
exec "$BIN" "$@"