#!/usr/bin/env python3
"""Count lines in all .AS and .AC files in the project tree."""

import os
import sys
import glob


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    as_files = sorted(glob.glob(os.path.join(project_root, "**", "*.AS"), recursive=True))
    ac_files = sorted(glob.glob(os.path.join(project_root, "**", "*.AC"), recursive=True))
    all_files = as_files + ac_files

    if not all_files:
        print("No .AS or .AC files found.")
        sys.exit(0)

    total_lines = 0
    for path in all_files:
        rel = os.path.relpath(path, project_root)
        try:
            with open(path, "r", errors="ignore") as f:
                lines = sum(1 for _ in f)
        except (OSError, IOError) as e:
            print(f"{'ERROR':>6}  {rel}  ({e})")
            continue
        total_lines += lines
        print(f"{lines:>6}  {rel}")

    print(f"\n{'-' * 50}")
    print(f"{total_lines:>6}  total lines in {len(all_files)} files")
    print(f"          ({len(as_files)} .AS, {len(ac_files)} .AC)")


if __name__ == "__main__":
    main()
