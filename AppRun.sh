#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
BIN="$HERE/usr/bin/radian"

export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
export LIBDECOR_PLUGIN_DIR="$HERE/usr/lib/libdecor/plugins-1"
export XDG_DATA_DIRS="$HERE/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# Force Wayland backend for FLTK 1.4
export FLTK_BACKEND=wayland

exec "$BIN" "$@"
