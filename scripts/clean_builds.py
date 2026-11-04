#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
"""Audit and clean legacy build directories while preserving active builds, caches, and user projects.

Defaults to Dry Run mode. Pass --apply (and --yes for non-interactive runs) to delete reclaimable directories.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

STANDARD_BUILDS = {
    "build-dev",
    "build-release",
    "build-core",
    "build-sanitize",
}

PROTECTED_OUT_SUBDIRS = {
    "deps",
    "factory05",
    "surge05",
    "community04",
    "dist",
}

RECLAIMABLE_OUT_SUBDIRS = {
    "0.2",
    "0.3",
    "0.4",
    "tests04",
    "testpacks04",
    "ui05",
    "plugin05",
    "native05",
    "premium05",
    "ko05",
}


def dir_size_bytes(path: pathlib.Path) -> int:
    total = 0
    if not path.exists():
        return 0
    for dirpath, dirnames, filenames in os.walk(path, followlinks=False):
        for name in dirnames:
            full = pathlib.Path(dirpath) / name
            if full.is_symlink():
                try:
                    total += full.lstat().st_size
                except OSError:
                    pass
        for name in filenames:
            full = pathlib.Path(dirpath) / name
            try:
                total += full.lstat().st_size
            except OSError:
                pass
    return total


def format_bytes(size: int) -> str:
    units = ["B", "KB", "MB", "GB", "TB"]
    value = float(size)
    for unit in units:
        if value < 1024.0 or unit == units[-1]:
            return f"{value:7.2f} {unit}"
        value /= 1024.0
    return f"{size} B"


def contains_user_project_files(path: pathlib.Path) -> bool:
    for dirpath, _, filenames in os.walk(path, followlinks=False):
        for fname in filenames:
            if fname.endswith(".reverie"):
                # Ignore automated test scratch projects inside known test output dirs
                rel = pathlib.Path(dirpath).resolve().relative_to(ROOT)
                if rel.parts and rel.parts[0] == "out" and len(rel.parts) > 1 and rel.parts[1] in RECLAIMABLE_OUT_SUBDIRS:
                    continue
                return True
    return False


def scan_workspace(include_active_builds: bool = False):
    protected: list[tuple[pathlib.Path, int, str]] = []
    reclaimable: list[tuple[pathlib.Path, int, str]] = []

    # 1. Check build* directories in repository root
    for entry in sorted(ROOT.iterdir()):
        if not entry.is_dir():
            continue
        name = entry.name
        if name == "build" or name.startswith("build-"):
            size = dir_size_bytes(entry)
            if contains_user_project_files(entry):
                protected.append((entry, size, "contains .reverie project files (protected)"))
            elif name in STANDARD_BUILDS and not include_active_builds:
                protected.append((entry, size, "standard reusable build directory"))
            elif name in STANDARD_BUILDS and include_active_builds:
                reclaimable.append((entry, size, "standard build directory (--include-active-builds)"))
            else:
                reclaimable.append((entry, size, "legacy version-specific build directory"))

    # 2. Check out/* directories
    out_dir = ROOT / "out"
    if out_dir.is_dir():
        loose_files_size = 0
        loose_files_count = 0
        for entry in sorted(out_dir.iterdir()):
            if not entry.is_dir():
                try:
                    loose_files_size += entry.lstat().st_size
                    loose_files_count += 1
                except OSError:
                    pass
                continue
            size = dir_size_bytes(entry)
            if entry.name in PROTECTED_OUT_SUBDIRS:
                protected.append((entry, size, "shared dependency/factory asset cache (protected)"))
            elif entry.name in RECLAIMABLE_OUT_SUBDIRS:
                reclaimable.append((entry, size, "temporary test/verification output"))
            else:
                protected.append((entry, size, "unclassified out/ directory (protected by default)"))
        if loose_files_count > 0:
            protected.append((out_dir / f"[{loose_files_count} loose files]", loose_files_size, "logs & archives in out/ (protected)"))

    # 3. Check dist/* directories
    dist_dir = ROOT / "dist"
    if dist_dir.is_dir():
        for entry in sorted(dist_dir.iterdir()):
            size = dir_size_bytes(entry)
            if entry.name == "current":
                protected.append((entry, size, "current standalone app distribution"))
            else:
                reclaimable.append((entry, size, "archived dist staging directory"))

    return protected, reclaimable


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=True,
        help="List directories and reclaimable disk space without deleting anything (default).",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Actually delete reclaimable legacy build and temporary test directories.",
    )
    parser.add_argument(
        "--yes",
        action="store_true",
        help="Skip interactive confirmation when --apply is specified.",
    )
    parser.add_argument(
        "--include-active-builds",
        action="store_true",
        help="Also include standard build-{dev,release,core,sanitize} directories in cleanup (never touches out/deps, out/factory05, or dist/current).",
    )
    args = parser.parse_args()

    dry_run = not args.apply
    protected, reclaimable = scan_workspace(include_active_builds=args.include_active_builds)

    mode_label = "DRY RUN (no files will be deleted)" if dry_run else "APPLY MODE"
    print(f"=== Reverie Ensemble Build & Artifact Audit [{mode_label}] ===")
    print(f"Workspace: {ROOT}\n")

    print("Protected / Active Directories (kept):")
    protected_total = 0
    if not protected:
        print("  (none)")
    for path, size, reason in protected:
        protected_total += size
        rel = path.relative_to(ROOT)
        print(f"  [KEEP]   {format_bytes(size)}  {str(rel):<28} ({reason})")
    print(f"  Subtotal Kept:        {format_bytes(protected_total)}\n")

    print("Reclaimable Legacy Builds & Temporary Outputs:")
    reclaim_total = 0
    if not reclaimable:
        print("  (none — workspace is clean)")
    for path, size, reason in reclaimable:
        reclaim_total += size
        rel = path.relative_to(ROOT)
        action = "WOULD DELETE" if dry_run else "DELETE"
        print(f"  [{action}] {format_bytes(size)}  {str(rel):<28} ({reason})")
    print(f"  Subtotal Reclaimable: {format_bytes(reclaim_total)}\n")

    if dry_run:
        if reclaimable:
            print("Dry run complete. To remove the reclaimable directories above, run:")
            print("  python3 scripts/clean_builds.py --apply")
        return 0

    if not reclaimable:
        print("Nothing to delete.")
        return 0

    if not args.yes:
        reply = input(f"Delete {len(reclaimable)} directories ({format_bytes(reclaim_total).strip()})? [y/N]: ").strip().lower()
        if reply not in ("y", "yes"):
            print("Aborted.")
            return 1

    freed = 0
    for path, size, _ in reclaimable:
        rel = path.relative_to(ROOT)
        print(f"Removing {rel} ({format_bytes(size).strip()})...")
        shutil.rmtree(path)
        freed += size

    print(f"Done. Freed {format_bytes(freed).strip()}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
