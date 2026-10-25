#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
# Explicit developer dependency setup. No build-time network downloads.
set -eu
cd "$(dirname "$0")/.."
if [ ! -d out/deps/sfizz/.git ]; then
  git clone --depth 1 --branch 1.2.3 --recurse-submodules --shallow-submodules https://github.com/sfztools/sfizz.git out/deps/sfizz
fi
[ "$(git -C out/deps/sfizz rev-parse HEAD)" = 4e70dc0bef53b41f2853ed46e26f5911114c92d0 ] || { echo "Unexpected sfizz revision" >&2; exit 1; }
# Apple clang 17+ rejects an obsolete dependent-template spelling in pinned atomic_queue.
python3 - <<'PY'
from pathlib import Path
p=Path('out/deps/sfizz/external/atomic_queue/include/atomic_queue/atomic_queue.h')
s=p.read_text().replace('Base::template do_pop_any(', 'Base::do_pop_any(').replace('Base::template do_push_any(', 'Base::do_push_any(')
p.write_text(s)
# sfizz 1.2.3 freewheeling waits only for already-dispatched jobs. A fast offline
# render can exhaust the preload before the dispatcher moves the pending queue.
# Drain under the same dispatcher mutex, offline only; live callbacks are unchanged.
p=Path('out/deps/sfizz/src/sfizz/FilePool.cpp')
s=p.read_text()
before='''void sfz::FilePool::waitForBackgroundLoading() noexcept
{
    std::lock_guard<std::mutex> guard { loadingJobsMutex };
'''
after=before+'''
    // Reverie: include requests not yet picked up by the dispatcher (offline only).
    QueuedFileData queuedData;
    while (filesToLoad->try_pop(queuedData)) {
        if (!queuedData.id.expired())
            loadingJobs.push_back(
                threadPool->enqueue([this](const QueuedFileData& data) { loadingJob(data); }, queuedData));
    }
'''
if '// Reverie: include requests not yet picked up' not in s:
    if s.count(before) != 1:
        raise SystemExit('Unexpected sfizz FilePool: inspect the freewheeling patch')
    p.write_text(s.replace(before,after))
PY
processor=$(uname -m)
if [ "$processor" = arm64 ]; then processor=aarch64; fi
cmake -S out/deps/sfizz -B out/deps/sfizz-build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/out/deps/install" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DPROJECT_SYSTEM_PROCESSOR="$processor" -DSFIZZ_JACK=OFF -DSFIZZ_RENDER=OFF \
  -DSFIZZ_TESTS=OFF -DENABLE_LTO=OFF
cmake --build out/deps/sfizz-build -j 6
cmake --install out/deps/sfizz-build
# Install the SF3 decoder dependency first on macOS: brew install libsndfile
if [ ! -d out/deps/fluidsynth/.git ]; then
  git clone --depth 1 --branch v2.6.1 https://github.com/FluidSynth/fluidsynth.git out/deps/fluidsynth
fi
[ "$(git -C out/deps/fluidsynth rev-parse HEAD)" = 71e85b2ca6bf48641ba7e2261d8f2640ae473773 ] || { echo "Unexpected FluidSynth revision" >&2; exit 1; }
cmake -S out/deps/fluidsynth -B out/deps/fluid-build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/out/deps/install" -DBUILD_TESTING=OFF \
  -Denable-readline=OFF -Denable-network=OFF -Denable-alsa=OFF -Denable-jack=OFF \
  -Denable-pipewire=OFF -Denable-pulseaudio=OFF -Denable-portaudio=OFF \
  -Denable-sdl3=OFF -Denable-coreaudio=OFF -Denable-coremidi=OFF -Denable-dbus=OFF \
  -Denable-ladspa=OFF -Denable-openmp=OFF -Denable-framework=OFF \
  -Denable-signalsmith=OFF -Denable-native-dls=OFF -Denable-aufile=OFF
cmake --build out/deps/fluid-build -j 6
cmake --install out/deps/fluid-build
