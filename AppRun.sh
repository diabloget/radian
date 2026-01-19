#!/bin/bash
HERE=$(dirname "$(readlink -f "$0")")
BIN="$HERE/usr/bin/radian"

# Copiar binario a /tmp si necesitamos permisos root
if [ "$EUID" -ne 0 ]; then
    TMP_BIN="/tmp/radian-$$"
    cp "$BIN" "$TMP_BIN"
    chmod +x "$TMP_BIN"
    
    if command -v pkexec >/dev/null 2>&1; then
        pkexec env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" "$TMP_BIN"
        RET=$?
    else
        x-terminal-emulator -e "sudo $TMP_BIN" || xterm -e "sudo $TMP_BIN"
        RET=$?
    fi
    
    rm -f "$TMP_BIN"
    exit $RET
fi

# Si ya somos root, ejecutar directamente
exec "$BIN" "$@"