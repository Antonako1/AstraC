#!/bin/sh
# Build AstraC and create a Linux release tarball (mirrors NSIS contents).
# Usage: CREATE_PACKAGE.sh [-nobuild] [-with-src]
#
# Output: build/AstraC-<version>-linux-<arch>.tar.gz

set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build"
VERSION=$(tr -d ' \t\r\n' < "$PROJECT_ROOT/VERSION/VERSION.txt")
ARCH=$(uname -m)
PKG_NAME="AstraC-${VERSION}-linux-${ARCH}"
STAGE_DIR="$BUILD_DIR/package/$PKG_NAME"
TARBALL="$BUILD_DIR/${PKG_NAME}.tar.gz"

NOBUILD=
WITH_SRC=
for arg in "$@"; do
    case "$arg" in
        -nobuild) NOBUILD=1 ;;
        -with-src) WITH_SRC=1 ;;
        -h|--help)
            echo "Usage: $0 [-nobuild] [-with-src]"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 1
            ;;
    esac
done

echo "============================================"
echo "AstraC Linux Package Builder"
echo "  version: $VERSION"
echo "  arch:    $ARCH"
echo "============================================"
echo

cd "$PROJECT_ROOT"

if [ -n "$NOBUILD" ]; then
    echo "[1/3] Skipping project build (-nobuild)"
else
    echo "[1/3] Building AstraC..."
    sh "$SCRIPT_DIR/MAKE.sh"
    echo "Build completed."
fi

BIN=
for candidate in \
    "$BUILD_DIR/AstraC" \
    "$BUILD_DIR/Release/AstraC" \
    "$BUILD_DIR/bin/AstraC"
do
    if [ -x "$candidate" ]; then
        BIN="$candidate"
        break
    fi
done

if [ -z "$BIN" ]; then
    echo "ERROR: AstraC binary not found under $BUILD_DIR. Build the project first." >&2
    exit 1
fi

echo
echo "[2/3] Staging package tree..."
rm -rf "$STAGE_DIR"
mkdir -p \
    "$STAGE_DIR/bin" \
    "$STAGE_DIR/share/astrac/icons" \
    "$STAGE_DIR/share/astrac/examples/AS" \
    "$STAGE_DIR/share/astrac/examples/AC" \
    "$STAGE_DIR/share/astrac/docs" \
    "$STAGE_DIR/share/applications" \
    "$STAGE_DIR/share/mime/packages"

cp "$BIN" "$STAGE_DIR/bin/AstraC"
chmod 755 "$STAGE_DIR/bin/AstraC"

cp "$PROJECT_ROOT/LICENSE" "$STAGE_DIR/share/astrac/LICENSE"
cp "$PROJECT_ROOT/NSIS/AC.ico" "$STAGE_DIR/share/astrac/icons/"
cp "$PROJECT_ROOT/NSIS/AH.ico" "$STAGE_DIR/share/astrac/icons/"
cp "$PROJECT_ROOT/NSIS/AS.ico" "$STAGE_DIR/share/astrac/icons/"

# Examples (same set as NSIS SecExamples)
cp "$PROJECT_ROOT/AS/BOOTLOADER.AS" "$STAGE_DIR/share/astrac/examples/AS/"
cp "$PROJECT_ROOT/AS/INCLUDE.as"    "$STAGE_DIR/share/astrac/examples/AS/"
cp "$PROJECT_ROOT/AS/MAIN.AS"       "$STAGE_DIR/share/astrac/examples/AS/"

cp "$PROJECT_ROOT/AC/INCLUDE.AH" "$STAGE_DIR/share/astrac/examples/AC/"
cp "$PROJECT_ROOT/AC/MAIN.AC"    "$STAGE_DIR/share/astrac/examples/AC/"
cp "$PROJECT_ROOT/AC/MAIN.AS"    "$STAGE_DIR/share/astrac/examples/AC/"
cp "$PROJECT_ROOT/AC/TEST.AC"    "$STAGE_DIR/share/astrac/examples/AC/"
cp "$PROJECT_ROOT/AC/TEST.AH"    "$STAGE_DIR/share/astrac/examples/AC/"
cp "$PROJECT_ROOT/AC/TEST.AS"    "$STAGE_DIR/share/astrac/examples/AC/"
cp "$PROJECT_ROOT/AC/TEST2.AC"   "$STAGE_DIR/share/astrac/examples/AC/"

# Documentation (same set as NSIS SecDocs)
for f in \
    AC.md AS.md AC_LANG.md DSM.md PREPROCESSOR.md AC_FILEHEADER.md \
    index.html AC.html AS.html DSM.html MNEMS.html \
    AC.png AH.png AS.png
do
    cp "$PROJECT_ROOT/DOCS/$f" "$STAGE_DIR/share/astrac/docs/"
done

# Desktop entry + MIME types (Linux equivalent of file associations)
cat > "$STAGE_DIR/share/applications/astrac.desktop" << EOF
[Desktop Entry]
Type=Application
Name=AstraC
GenericName=AstraC Compiler, Assembler and Disassembler
Comment=Compile .AC, assemble .AS, and disassemble binaries
Exec=AstraC %F
Icon=astrac
Terminal=true
Categories=Development;IDE;
MimeType=text/x-astrac-ac;text/x-astrac-ah;text/x-astrac-as;
Keywords=assembler;compiler;x86;
EOF

cat > "$STAGE_DIR/share/mime/packages/astrac.xml" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="text/x-astrac-ac">
    <comment>AstraC AC source</comment>
    <glob pattern="*.ac"/>
    <glob pattern="*.AC"/>
  </mime-type>
  <mime-type type="text/x-astrac-ah">
    <comment>AstraC AH header</comment>
    <glob pattern="*.ah"/>
    <glob pattern="*.AH"/>
  </mime-type>
  <mime-type type="text/x-astrac-as">
    <comment>AstraC AS assembly</comment>
    <glob pattern="*.as"/>
    <glob pattern="*.AS"/>
  </mime-type>
</mime-info>
EOF

# Optional source tree (same as NSIS SecSource)
if [ -n "$WITH_SRC" ]; then
    echo "  including source tree..."
    SRC="$STAGE_DIR/share/astrac/src"
    mkdir -p \
        "$SRC/ASSEMBLER" "$SRC/COMPILER" "$SRC/DISSASEMBLER" \
        "$SRC/SHARED" "$SRC/STDLIB" "$SRC/VERSION" \
        "$SRC/NSIS" "$SRC/SCRIPTS/WIN" "$SRC/SCRIPTS/SH" "$SRC/DOCS"

    cp "$PROJECT_ROOT/ASTRAC.c" "$PROJECT_ROOT/ASTRAC.h" "$PROJECT_ROOT/ASTRAC.rc.in" \
       "$PROJECT_ROOT/LICENSE" "$PROJECT_ROOT/CMakeLists.txt" "$SRC/"
    cp "$PROJECT_ROOT/ASSEMBLER/"*.c "$PROJECT_ROOT/ASSEMBLER/"*.h "$SRC/ASSEMBLER/" 2>/dev/null || true
    cp "$PROJECT_ROOT/COMPILER/"*.c "$PROJECT_ROOT/COMPILER/"*.h "$SRC/COMPILER/" 2>/dev/null || true
    cp "$PROJECT_ROOT/DISSASEMBLER/"*.c "$PROJECT_ROOT/DISSASEMBLER/"*.h "$SRC/DISSASEMBLER/" 2>/dev/null || true
    cp "$PROJECT_ROOT/SHARED/"*.c "$SRC/SHARED/" 2>/dev/null || true
    cp "$PROJECT_ROOT/STDLIB/"*.c "$PROJECT_ROOT/STDLIB/"*.h "$SRC/STDLIB/" 2>/dev/null || true
    cp "$PROJECT_ROOT/VERSION/VERSION.h" "$PROJECT_ROOT/VERSION/VERSION.txt" "$SRC/VERSION/"
    cp "$PROJECT_ROOT/NSIS/"*.ico "$PROJECT_ROOT/NSIS/INSTALLER.NSI" "$SRC/NSIS/"
    cp "$PROJECT_ROOT/SCRIPTS/WIN/CREATE_NSIS.BAT" "$SRC/SCRIPTS/WIN/"
    cp "$PROJECT_ROOT/SCRIPTS/SH/"*.sh "$SRC/SCRIPTS/SH/"
    cp "$PROJECT_ROOT/DOCS/"*.md "$PROJECT_ROOT/DOCS/"*.html "$PROJECT_ROOT/DOCS/"*.png "$SRC/DOCS/" 2>/dev/null || true
fi

# Installer / uninstaller shipped inside the tarball
cp "$SCRIPT_DIR/install.sh"   "$STAGE_DIR/install.sh"
cp "$SCRIPT_DIR/uninstall.sh" "$STAGE_DIR/uninstall.sh"
chmod 755 "$STAGE_DIR/install.sh" "$STAGE_DIR/uninstall.sh"

# Manifest for uninstall
{
    echo "VERSION=$VERSION"
    echo "ARCH=$ARCH"
    find "$STAGE_DIR" -type f ! -name install.sh ! -name uninstall.sh ! -name MANIFEST \
        | sed "s|^$STAGE_DIR/||" | sort
} > "$STAGE_DIR/MANIFEST"

echo
echo "[3/3] Creating tarball..."
mkdir -p "$BUILD_DIR"
(
    cd "$BUILD_DIR/package"
    tar -czf "$TARBALL" "$PKG_NAME"
)

echo
echo "============================================"
echo "Done!"
echo "  $TARBALL"
echo "Install with:"
echo "  tar -xzf $TARBALL"
echo "  cd $PKG_NAME && sudo ./install.sh"
echo "============================================"
