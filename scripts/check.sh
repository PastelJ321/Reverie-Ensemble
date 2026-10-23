#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
set -eu
cd "$(dirname "$0")/.."
cmake -S . -B build-core -DREVERIE_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core -j 4
ctest --test-dir build-core --output-on-failure
