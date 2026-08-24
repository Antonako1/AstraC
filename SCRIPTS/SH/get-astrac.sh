#!/bin/sh
# One-line installer for AstraC on Linux.
#
#   curl -fsSL https://raw.githubusercontent.com/Antonako1/AstraC/main/SCRIPTS/SH/get-astrac.sh | sh
#
# Options (env or args after `sh -s --`):
#   ASTRAC_VERSION=0.1.1   pin a release (default: latest GitHub release)
#   ASTRAC_PREFIX=/usr/local
#   ASTRAC_USER=1          install to ~/.local
#   --user / --prefix DIR / --no-path / --no-mime  (passed to install.sh)
#
# Falls back to building from source if no matching release asset exists.

set -e

REPO="Antonako1/AstraC"
RAW_BASE="https://raw.githubusercontent.com/${REPO}"
API_BASE="https://api.github.com/repos/${REPO}"
RELEASES_BASE="https://github.com/${REPO}/releases/download"

VERSION="${ASTRAC_VERSION:-}"
PREFIX="${ASTRAC_PREFIX:-}"
USER_INSTALL="${ASTRAC_USER:-}"
EXTRA_ARGS=
FROM_SOURCE=

while [ $# -gt 0 ]; do
    case "$1" in
        --version) VERSION=$2; shift 2 ;;
        --version=*) VERSION=${1#--version=}; shift ;;
        --prefix) PREFIX=$2; EXTRA_ARGS="$EXTRA_ARGS --prefix $2"; shift 2 ;;
        --prefix=*) PREFIX=${1#--prefix=}; EXTRA_ARGS="$EXTRA_ARGS --prefix $PREFIX"; shift ;;
        --user) USER_INSTALL=1; EXTRA_ARGS="$EXTRA_ARGS --user"; shift ;;
        --no-path) EXTRA_ARGS="$EXTRA_ARGS --no-path"; shift ;;
        --no-mime) EXTRA_ARGS="$EXTRA_ARGS --no-mime"; shift ;;
        --from-source) FROM_SOURCE=1; shift ;;
        -h|--help)
            cat << 'EOF'
Usage: get-astrac.sh [options]

  --version X.Y.Z   Install a specific release (default: latest)
  --prefix DIR      Install root
  --user            Install to ~/.local
  --no-path         Skip PATH setup
  --no-mime         Skip desktop/MIME associations
  --from-source     Clone repo and build instead of downloading a tarball

Curl one-liner:
  curl -fsSL https://raw.githubusercontent.com/Antonako1/AstraC/main/SCRIPTS/SH/get-astrac.sh | sh
  curl -fsSL ... | sh -s -- --user
EOF
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "ERROR: required command not found: $1" >&2
        exit 1
    fi
}

need_cmd curl
need_cmd tar
need_cmd uname

ARCH=$(uname -m)
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
if [ "$OS" != "linux" ]; then
    echo "ERROR: this installer targets Linux (detected: $OS)." >&2
    exit 1
fi

TMPDIR_ROOT=${TMPDIR:-/tmp}
WORK=$(mktemp -d "$TMPDIR_ROOT/astrac-install.XXXXXX")
cleanup() { rm -rf "$WORK"; }
trap cleanup EXIT INT HUP TERM

download() {
    # $1=url $2=out
    curl -fsSL --retry 3 --retry-delay 1 -o "$2" "$1"
}

resolve_latest_version() {
    # Prefer GitHub latest release tag; fall back to VERSION.txt on main
    tag=$(curl -fsSL "$API_BASE/releases/latest" 2>/dev/null \
        | sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' \
        | head -n1)
    if [ -n "$tag" ]; then
        echo "$tag" | sed 's/^v//'
        return
    fi
    curl -fsSL "$RAW_BASE/main/VERSION/VERSION.txt" | tr -d ' \t\r\n'
}

install_from_tarball() {
    ver=$1
    pkg="AstraC-${ver}-linux-${ARCH}"
    url="${RELEASES_BASE}/v${ver}/${pkg}.tar.gz"
    alt_url="${RELEASES_BASE}/${ver}/${pkg}.tar.gz"
    tarball="$WORK/${pkg}.tar.gz"

    echo "Downloading $pkg ..."
    if download "$url" "$tarball" 2>/dev/null; then
        :
    elif download "$alt_url" "$tarball" 2>/dev/null; then
        :
    else
        return 1
    fi

    echo "Extracting..."
    tar -xzf "$tarball" -C "$WORK"
    # tarball may contain top-level dir
    if [ -d "$WORK/$pkg" ]; then
        STAGE="$WORK/$pkg"
    else
        STAGE=$(find "$WORK" -maxdepth 2 -type f -name install.sh -exec dirname {} \; | head -n1)
    fi
    if [ -z "$STAGE" ] || [ ! -x "$STAGE/install.sh" ]; then
        echo "ERROR: install.sh missing from package." >&2
        exit 1
    fi

    echo "Running installer..."
    # shellcheck disable=SC2086
    sh "$STAGE/install.sh" $EXTRA_ARGS
}

install_from_source() {
    need_cmd cmake
    if ! command -v ninja >/dev/null 2>&1 && ! command -v make >/dev/null 2>&1; then
        echo "ERROR: need ninja or make to build from source." >&2
        exit 1
    fi
    need_cmd cc
    need_cmd git

    echo "Cloning ${REPO}..."
    git clone --depth 1 "https://github.com/${REPO}.git" "$WORK/src"
    cd "$WORK/src"

    if [ -n "$VERSION" ] && [ "$VERSION" != "main" ] && [ "$VERSION" != "latest" ]; then
        git fetch --depth 1 origin "refs/tags/v${VERSION}:refs/tags/v${VERSION}" 2>/dev/null \
            || git fetch --depth 1 origin "refs/tags/${VERSION}:refs/tags/${VERSION}" 2>/dev/null \
            || true
        git checkout "v${VERSION}" 2>/dev/null || git checkout "${VERSION}" 2>/dev/null || true
    fi

    echo "Building package..."
    sh SCRIPTS/SH/CREATE_PACKAGE.sh
    ver=$(tr -d ' \t\r\n' < VERSION/VERSION.txt)
    tarball="build/AstraC-${ver}-linux-${ARCH}.tar.gz"
    if [ ! -f "$tarball" ]; then
        echo "ERROR: package not produced: $tarball" >&2
        exit 1
    fi
    tar -xzf "$tarball" -C "$WORK"
    STAGE="$WORK/AstraC-${ver}-linux-${ARCH}"
    # shellcheck disable=SC2086
    sh "$STAGE/install.sh" $EXTRA_ARGS
}

echo "============================================"
echo "AstraC Linux installer"
echo "  arch: $ARCH"
echo "============================================"
echo

if [ -n "$FROM_SOURCE" ]; then
    install_from_source
    exit 0
fi

if [ -z "$VERSION" ] || [ "$VERSION" = "latest" ]; then
    echo "Resolving latest version..."
    VERSION=$(resolve_latest_version)
fi
echo "Version: $VERSION"
echo

if ! install_from_tarball "$VERSION"; then
    echo
    echo "No prebuilt tarball for ${VERSION}/${ARCH}; building from source..."
    install_from_source
fi
