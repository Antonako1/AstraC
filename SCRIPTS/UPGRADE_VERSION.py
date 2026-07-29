#!/usr/bin/env python3
"""
Usage: python3 UPGRADE_VERSION.py [<major> <minor> <patch>] | [new_major] | [new_minor] | [new_patch]

major: 1 to increase major version number, 0 to keep it the same
minor: 1 to increase minor version number, 0 to keep it the same
patch: 1 to increase patch version number, 0 to keep it the same

new_major: Increments the major version number and resets minor and patch to 0
new_minor: Increments the minor version number and resets patch to 0
new_patch: Increments the patch version number

Files to update:
VERSION/VERSION.h           - Defined as #define VERSION 'x.y.z'
VERSION/VERSION.txt         - Contains the version number in plain text
VERSION/VERSION.nsh         - Defined as !define PRODUCT_VERSION "0.1.0"
"""
import os
import sys
import glob
import re



def main():
    # Get the current version from VERSION/VERSION.h
    version_file = os.path.join("VERSION", "VERSION.h")
    with open(version_file, "r") as f:
        content = f.read()
        match = re.search(r'#define VERSION\s+"(\d+)\.(\d+)\.(\d+)"', content)
        if not match:
            print("Error: Could not find version in VERSION/VERSION.h")
            sys.exit(1)
        major, minor, patch = map(int, match.groups())

    # Determine the new version based on command line arguments
    if len(sys.argv) == 4:
        major_inc, minor_inc, patch_inc = map(int, sys.argv[1:])
        major += major_inc
        minor += minor_inc
        patch += patch_inc
    elif len(sys.argv) == 2:
        arg = sys.argv[1]
        if arg == "new_major":
            major += 1
            minor = 0
            patch = 0
        elif arg == "new_minor":
            minor += 1
            patch = 0
        elif arg == "new_patch":
            patch += 1
        else:
            print("Error: Invalid argument. Use 'new_major', 'new_minor', or 'new_patch'.")
            sys.exit(1)
    else:
        print("Usage: python3 UPGRADE_VERSION.py [<major> <minor> <patch>] | [new_major] | [new_minor] | [new_patch]")
        sys.exit(1)

    new_version = f"{major}.{minor}.{patch}"

    # Update VERSION/VERSION.h
    with open(version_file, "w") as f:
        f.write(f'#define VERSION "{new_version}"\n')

    # Update VERSION/VERSION.txt
    version_txt_file = os.path.join("VERSION", "VERSION.txt")
    with open(version_txt_file, "w") as f:
        f.write(new_version + "\n")

    # Update VERSION/VERSION.nsh
    version_nsh_file = os.path.join("VERSION", "VERSION.nsh")
    with open(version_nsh_file, "w") as f:
        f.write(f'!define PRODUCT_VERSION "{new_version}"\n')

    print(f"Version updated to {new_version}")

if __name__ == "__main__":
    main()