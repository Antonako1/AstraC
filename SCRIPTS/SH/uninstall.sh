#!/bin/sh
# Remove an AstraC installation created by install.sh.
# Usage: uninstall.sh [--prefix DIR]
#   If omitted, reads PREFIX from install.conf next to this script,
#   or defaults to /usr/local.

set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PREFIX=
CONF=""

# Prefer conf beside this script (installed location: share/astrac/uninstall.sh)
if [ -f "$SCRIPT_DIR/install.conf" ]; then
    CONF="$SCRIPT_DIR/install.conf"
elif [ -f "$SCRIPT_DIR/share/astrac/install.conf" ]; then
    CONF="$SCRIPT_DIR/share/astrac/install.conf"
fi

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix) PREFIX=$2; shift 2 ;;
        --prefix=*) PREFIX=${1#--prefix=}; shift ;;
        -h|--help)
            echo "Usage: $0 [--prefix DIR]"
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

if [ -z "$PREFIX" ] && [ -n "$CONF" ]; then
    # shellcheck disable=SC1090
    PREFIX=$(grep '^PREFIX=' "$CONF" | head -n1 | cut -d= -f2-)
fi
[ -n "$PREFIX" ] || PREFIX="/usr/local"

if [ ! -e "$PREFIX/bin/AstraC" ] && [ ! -d "$PREFIX/share/astrac" ]; then
    echo "ERROR: AstraC does not appear to be installed under $PREFIX" >&2
    exit 1
fi

if [ ! -w "$PREFIX/bin" ] 2>/dev/null && [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: cannot write to $PREFIX (try sudo)." >&2
    exit 1
fi

echo "Uninstalling AstraC from $PREFIX ..."

rm -f "$PREFIX/bin/AstraC"
rm -f "$PREFIX/share/applications/astrac.desktop"
rm -f "$PREFIX/share/mime/packages/astrac.xml"
rm -f "$PREFIX/share/icons/hicolor/256x256/apps/astrac.png"
rm -f /etc/profile.d/astrac.sh 2>/dev/null || true

# Remove PATH markers from user shell configs
for rc in "$HOME/.profile" "$HOME/.bashrc" "$HOME/.zshrc"; do
    [ -f "$rc" ] || continue
    if grep -F "AstraC PATH" "$rc" >/dev/null 2>&1; then
        tmp=$(mktemp 2>/dev/null || echo "/tmp/astrac-uninst.$$")
        # Drop the marker comment and the following PATH line
        awk '
            /# AstraC PATH/ { skip=1; next }
            skip==1 && /^export PATH=/ { skip=0; next }
            { skip=0; print }
        ' "$rc" > "$tmp" && mv "$tmp" "$rc"
        echo "  cleaned PATH entry in $rc"
    fi
done

rm -rf "$PREFIX/share/astrac"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$PREFIX/share/applications" 2>/dev/null || true
fi
if command -v update-mime-database >/dev/null 2>&1; then
    update-mime-database "$PREFIX/share/mime" 2>/dev/null || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q "$PREFIX/share/icons/hicolor" 2>/dev/null || true
fi

echo "AstraC removed from $PREFIX."
