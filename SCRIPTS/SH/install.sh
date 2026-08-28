#!/bin/sh
# Install AstraC from an extracted release tarball.
# Mirrors NSIS: binary, icons, LICENSE, examples, docs, PATH, associations.
#
# Usage:
#   ./install.sh [--prefix DIR] [--user] [--no-path] [--no-mime] [--with-src]
#
# Defaults:
#   system install → /usr/local  (needs root)
#   --user         → ~/.local

set -e

PREFIX=
USER_INSTALL=
ADD_PATH=1
ADD_MIME=1
INSTALL_SRC=

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix) PREFIX=$2; shift 2 ;;
        --prefix=*) PREFIX=${1#--prefix=}; shift ;;
        --user) USER_INSTALL=1; shift ;;
        --no-path) ADD_PATH=; shift ;;
        --no-mime) ADD_MIME=; shift ;;
        --with-src) INSTALL_SRC=1; shift ;;
        -h|--help)
            cat << 'EOF'
Usage: ./install.sh [options]

  --prefix DIR   Install root (default: /usr/local, or ~/.local with --user)
  --user         Install for current user only (no root required)
  --no-path      Do not add bin/ to shell PATH configs
  --no-mime      Do not install desktop entry / MIME associations
  --with-src     Install source tree if present in the package
EOF
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

PKG_ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cd "$PKG_ROOT"

if [ ! -x "$PKG_ROOT/bin/AstraC" ]; then
    echo "ERROR: bin/AstraC not found. Run this from an extracted AstraC package." >&2
    exit 1
fi

if [ -z "$PREFIX" ]; then
    if [ -n "$USER_INSTALL" ]; then
        PREFIX="${HOME}/.local"
    else
        PREFIX="/usr/local"
    fi
fi

# Need write access to prefix
if [ ! -w "$(dirname "$PREFIX")" ] 2>/dev/null || \
   { [ -d "$PREFIX" ] && [ ! -w "$PREFIX" ]; }; then
    if [ "$(id -u)" -ne 0 ]; then
        echo "ERROR: cannot write to $PREFIX (try sudo, or --user / --prefix)." >&2
        exit 1
    fi
fi

VERSION=
if [ -f "$PKG_ROOT/MANIFEST" ]; then
    VERSION=$(grep '^VERSION=' "$PKG_ROOT/MANIFEST" | head -n1 | cut -d= -f2-)
fi
[ -n "$VERSION" ] || VERSION="unknown"

echo "Installing AstraC ${VERSION} → ${PREFIX}"
echo

mkdir -p \
    "$PREFIX/bin" \
    "$PREFIX/share/astrac/icons" \
    "$PREFIX/share/astrac/examples/AS" \
    "$PREFIX/share/astrac/examples/AC" \
    "$PREFIX/share/astrac/docs" \
    "$PREFIX/share/applications" \
    "$PREFIX/share/mime/packages"

install -m 755 "$PKG_ROOT/bin/AstraC" "$PREFIX/bin/AstraC"
install -m 644 "$PKG_ROOT/share/astrac/LICENSE" "$PREFIX/share/astrac/LICENSE"

for ico in AC.ico AH.ico AS.ico; do
    install -m 644 "$PKG_ROOT/share/astrac/icons/$ico" "$PREFIX/share/astrac/icons/$ico"
done

# Examples
for f in BOOTLOADER.AS INCLUDE.as MAIN.AS; do
    install -m 644 "$PKG_ROOT/share/astrac/examples/AS/$f" \
        "$PREFIX/share/astrac/examples/AS/$f"
done
for f in INCLUDE.AH MAIN.AC MAIN.AS TEST.AC TEST.AH TEST.AS TEST2.AC; do
    install -m 644 "$PKG_ROOT/share/astrac/examples/AC/$f" \
        "$PREFIX/share/astrac/examples/AC/$f"
done

# Docs
for f in \
    AC.md AS.md AC_LANG.md DSM.md PREPROCESSOR.md AC_FILEHEADER.md \
    index.html AC.html AS.html DSM.html MNEMS.html \
    AC.png AH.png AS.png
do
    install -m 644 "$PKG_ROOT/share/astrac/docs/$f" \
        "$PREFIX/share/astrac/docs/$f"
done

if [ -n "$INSTALL_SRC" ] && [ -d "$PKG_ROOT/share/astrac/src" ]; then
    echo "  installing source tree..."
    mkdir -p "$PREFIX/share/astrac/src"
    cp -a "$PKG_ROOT/share/astrac/src/." "$PREFIX/share/astrac/src/"
fi

# Desktop + MIME (Linux stand-in for NSIS file associations)
if [ -n "$ADD_MIME" ]; then
    install -m 644 "$PKG_ROOT/share/applications/astrac.desktop" \
        "$PREFIX/share/applications/astrac.desktop"
    install -m 644 "$PKG_ROOT/share/mime/packages/astrac.xml" \
        "$PREFIX/share/mime/packages/astrac.xml"

    # Prefer PNG icons for freedesktop; fall back to copying docs icons
    ICON_DIR="$PREFIX/share/icons/hicolor/256x256/apps"
    mkdir -p "$ICON_DIR"
    if [ -f "$PKG_ROOT/share/astrac/docs/AC.png" ]; then
        install -m 644 "$PKG_ROOT/share/astrac/docs/AC.png" "$ICON_DIR/astrac.png"
    fi

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$PREFIX/share/applications" 2>/dev/null || true
    fi
    if command -v update-mime-database >/dev/null 2>&1; then
        update-mime-database "$PREFIX/share/mime" 2>/dev/null || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q "$PREFIX/share/icons/hicolor" 2>/dev/null || true
    fi
fi

# Uninstaller + install record
install -m 755 "$PKG_ROOT/uninstall.sh" "$PREFIX/share/astrac/uninstall.sh"
{
    echo "PREFIX=$PREFIX"
    echo "VERSION=$VERSION"
    echo "USER_INSTALL=${USER_INSTALL:-0}"
    echo "ADD_PATH=${ADD_PATH:-0}"
    echo "ADD_MIME=${ADD_MIME:-0}"
    date -u +'INSTALLED=%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || true
} > "$PREFIX/share/astrac/install.conf"

# PATH helpers (system profile.d or user shell rc)
PATH_LINE="export PATH=\"${PREFIX}/bin:\$PATH\""
if [ -n "$ADD_PATH" ]; then
    if [ -n "$USER_INSTALL" ] || [ "$PREFIX" = "${HOME}/.local" ]; then
        for rc in "$HOME/.profile" "$HOME/.bashrc" "$HOME/.zshrc"; do
            [ -f "$rc" ] || continue
            if ! grep -F "AstraC PATH" "$rc" >/dev/null 2>&1; then
                {
                    echo ""
                    echo "# AstraC PATH"
                    echo "$PATH_LINE"
                } >> "$rc"
                echo "  PATH entry added to $rc"
            fi
        done
        # Ensure at least one rc exists for login shells
        if [ ! -f "$HOME/.profile" ]; then
            {
                echo "# AstraC PATH"
                echo "$PATH_LINE"
            } > "$HOME/.profile"
            echo "  PATH entry added to $HOME/.profile"
        fi
    else
        PROFILE_D="/etc/profile.d/astrac.sh"
        if [ -d /etc/profile.d ] && { [ -w /etc/profile.d ] || [ "$(id -u)" -eq 0 ]; }; then
            cat > "$PROFILE_D" << EOF
# AstraC PATH
export PATH="${PREFIX}/bin:\$PATH"
EOF
            chmod 644 "$PROFILE_D"
            echo "  PATH entry written to $PROFILE_D"
        fi
    fi
fi

echo
echo "AstraC ${VERSION} installed."
echo "  binary:  $PREFIX/bin/AstraC"
echo "  docs:    $PREFIX/share/astrac/docs"
echo "  examples:$PREFIX/share/astrac/examples"
echo "  remove:  $PREFIX/share/astrac/uninstall.sh"
if [ -n "$ADD_PATH" ]; then
    echo
    echo "Open a new shell (or: export PATH=\"${PREFIX}/bin:\$PATH\") then run: AstraC version"
fi
