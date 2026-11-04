#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

JOBS="${REVERIE_BUILD_JOBS:-4}"
PROFILE="${1:-dev}"

usage() {
  cat <<'EOF'
Usage: scripts/build.sh [dev|release|core|sanitize|dist|all-tests]

Profiles:
  dev        Configure & build GUI in build-dev/ (Debug, symlinked FactoryLibrary)
  release    Configure & build GUI in build-release/ (Release, symlinked FactoryLibrary)
  core       Configure & build core engine in build-core/ (headless)
  sanitize   Configure & build with ASan/UBSan in build-sanitize/
  dist       Stage standalone ReverieEnsemble.app into dist/current/ with bundled resources & dylibs
  all-tests  Run CTest suites across build-core, build-dev, build-release, and build-sanitize
EOF
}

build_dev() {
  cmake -S . -B build-dev \
    -DCMAKE_BUILD_TYPE=Debug \
    -DREVERIE_BUILD_GUI=ON \
    -DREVERIE_BUNDLE_FACTORY_COPY=OFF
  cmake --build build-dev -j "$JOBS"
}

build_release() {
  cmake -S . -B build-release \
    -DCMAKE_BUILD_TYPE=Release \
    -DREVERIE_BUILD_GUI=ON \
    -DREVERIE_BUNDLE_FACTORY_COPY=OFF
  cmake --build build-release -j "$JOBS"
}

build_core() {
  cmake -S . -B build-core \
    -DCMAKE_BUILD_TYPE=Debug \
    -DREVERIE_BUILD_GUI=OFF
  cmake --build build-core -j "$JOBS"
}

build_sanitize() {
  cmake -S . -B build-sanitize \
    -DCMAKE_BUILD_TYPE=Debug \
    -DREVERIE_BUILD_GUI=OFF \
    -DREVERIE_SANITIZE=ON
  cmake --build build-sanitize -j "$JOBS"
}

stage_dist() {
  build_release
  local app_src="build-release/ReverieEnsemble.app"
  local dist_dir="dist/current"
  local app_dst="$dist_dir/ReverieEnsemble.app"

  mkdir -p "$dist_dir"
  rm -rf "$app_dst"
  cp -R "$app_src" "$app_dst"

  # Replace symlinked FactoryLibrary with standalone copy inside dist/current/ReverieEnsemble.app
  if [ -L "$app_dst/Contents/Resources/FactoryLibrary" ] || [ -d "$app_dst/Contents/Resources/FactoryLibrary" ]; then
    rm -rf "$app_dst/Contents/Resources/FactoryLibrary"
  fi
  if [ -f "out/factory05/pack.json" ]; then
    cp -R "out/factory05" "$app_dst/Contents/Resources/FactoryLibrary"
  fi

  # Bundle sfizz and fluidsynth dynamic libraries into Contents/Frameworks so dist bundle has zero build-path dependencies
  if [ "$(uname -s)" = "Darwin" ] && [ -d "out/deps/install/lib" ]; then
    local fw_dir="$app_dst/Contents/Frameworks"
    mkdir -p "$fw_dir"
    cp -P out/deps/install/lib/libsfizz*.dylib "$fw_dir/" 2>/dev/null || true
    cp -P out/deps/install/lib/libfluidsynth*.dylib "$fw_dir/" 2>/dev/null || true

    local abs_lib_dir="$ROOT_DIR/out/deps/install/lib"
    for dylib in "$fw_dir"/*.dylib; do
      [ -e "$dylib" ] || continue
      if [ ! -L "$dylib" ]; then
        local base
        base="$(basename "$dylib")"
        install_name_tool -id "@rpath/$base" "$dylib" || true
      fi
    done

    for bin in "$app_dst/Contents/MacOS/ReverieEnsemble" "$app_dst/Contents/MacOS/reverie-plugin-scan"; do
      [ -f "$bin" ] || continue
      install_name_tool -delete_rpath "$abs_lib_dir" "$bin" 2>/dev/null || true
      install_name_tool -add_rpath "@executable_path/../Frameworks" "$bin" 2>/dev/null || true
      install_name_tool -change "$abs_lib_dir/libfluidsynth.3.dylib" "@rpath/libfluidsynth.3.dylib" "$bin" 2>/dev/null || true
      install_name_tool -change "$abs_lib_dir/libsfizz.1.dylib" "@rpath/libsfizz.1.dylib" "$bin" 2>/dev/null || true
    done

    codesign --force --deep --sign - "$app_dst" >/dev/null 2>&1 || true
  fi

  echo "Staged standalone bundle at $app_dst"
}

run_all_tests() {
  for dir in build-core build-dev build-release build-sanitize; do
    if [ -d "$dir" ]; then
      echo "=== Running CTest in $dir ==="
      ctest --test-dir "$dir" --output-on-failure
    fi
  done
}

case "$PROFILE" in
  dev)       build_dev ;;
  release)   build_release ;;
  core)      build_core ;;
  sanitize)  build_sanitize ;;
  dist)      stage_dist ;;
  all-tests) run_all_tests ;;
  -h|--help) usage ;;
  *)
    usage >&2
    exit 1
    ;;
esac
