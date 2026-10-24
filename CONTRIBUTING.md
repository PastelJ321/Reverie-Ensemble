# Contributing

Copyright 2026 QuadraJ and contributors.

Contributions to Reverie Ensemble's original source are under Apache-2.0. Preserve third-party licenses and copyright notices. Do not add GPL/AGPL code, extracted commercial DAW samples, unidentified sound packs, runtime downloads, or account/cloud requirements to the core workflow. A new optional LGPL instrument backend must be separately documented and dynamically linked.

Build and run CTest using README.md. Keep `reverie_core` independent of Qt. Every audio-thread operation must have bounded execution and must avoid allocation/deallocation, filesystem/network I/O, blocking locks/logging, and UI access. Prepare state outside the callback and return ownership through the retirement queue. Stop/join a device before destroying an engine/recorder it references.

Use C++20 and the project's `.clang-format`. On macOS:

```sh
xcrun clang-format -i src/core/*.cpp src/core/*.h src/ui/*.cpp src/ui/*.h
cmake --build build-03-release -j 4
ctest --test-dir build-03-release --output-on-failure
```

For DSP, ownership, or parser changes also run the sanitizer configuration in docs/TESTING.md. Test meaningful user behavior: sample timing, note termination, render identity, project round trips, missing media and editing gestures. Do not add UI controls before their actual engine/model behavior exists.

Describe the user-visible problem, resulting behavior, validation performed, known limits and dependency/license changes in pull requests. Do not claim hardware testing when only offline tests were run. Samples require an explicit source URL, copyright/license notice and redistribution permission; prefer original synthesis or CC0 recordings.

No contributor agreement or logo-display obligation is imposed by this repository.
