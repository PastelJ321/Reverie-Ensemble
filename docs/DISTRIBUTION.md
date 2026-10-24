# macOS distribution and relinking

The current verified artifact is a development `.app` linked to installed shared Qt frameworks. It runs with the dependencies described in README.md; it is not yet a standalone/notarized download. Do not distribute it as though all bundled code were Apache-2.0.

For a public 0.3 binary release:

1. Build Release with an explicit deployment target and Qt architecture matching the intended Macs. This session tested arm64 only. Intel requires x86_64 Qt and a matching build; universal requires both architectures for **every** runtime dependency. Do not set a deployment target older than the Qt build supports.
2. Stage a copy of the `.app`; run the selected Qt installation's `macdeployqt` on that staging copy. This preserves development build paths and gathers shared frameworks/Cocoa plugins. Audit with `otool -L` recursively to ensure no unintended Homebrew absolute runtime paths remain. Omit unnecessary plugins/modules and confirm that every included module permits the selected LGPL distribution.
3. Preserve the bundled `Resources/assets` directory (self-contained IBM Plex fonts/OFL notices and Community Essentials and Ensemble Essentials WAV/SFZ/CC0 manifests). Include `docs/SOUND_LIBRARY_LICENSE_MANIFEST.md` with source/credit details; user-installed packs retain their own notices and are not automatically redistributable. Place LICENSE, NOTICE, THIRD_PARTY_LICENSES.md, all direct and transitive license/copyright notices and relevant Qt SBOM in the bundle's Resources/Legal and alongside the download. Our CMake already embeds original/direct license notices; it cannot infer all third-party notices from an arbitrary Qt installation.
4. Prepare exact corresponding source archives for the Qt/LGPL libraries actually shipped, plus modifications, platform patches, dependency sources and build configurations as required. Offer them through the same release download in a license-compliant way. The tested Homebrew Qt source is `qtbase-everywhere-src-6.11.2.tar.xz` at the [official Qt archive](https://download.qt.io/official_releases/qt/6.11/6.11.2/submodules/), with Homebrew formula SHA-256 `5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22`. Capture the corresponding formula/build flags/patches and exact dependencies; an upstream archive alone does not describe every Homebrew binary component.
5. Document relinking/replacement of the Qt frameworks and plugins. Supply this application's source/build scripts. A recipient can alternatively rebuild the entire app against their compatible Qt prefix using `CMAKE_PREFIX_PATH`. After replacement in a staged bundle, local ad-hoc sign nested changed libraries/app as needed (`codesign --force --deep --sign - <app>`). Do not impose restrictions on debugging changes to the LGPL libraries. Preserve installation information required by the chosen platform/distribution.
6. Sign the distributable with Developer ID, proper microphone usage metadata (already in Info.plist) and appropriate hardened-runtime settings; notarize/staple and test quarantine launch, microphone allow/deny, device failure and loading/saving with a clean nondeveloper account. Neither signing nor notarization was performed in this session. Do not add networking/accounts to enable local features.
7. Re-run the device and manual workflow checklist in TESTING.md on both target CPU architectures. Retain the exact sources, SBOM, notices and validation report for each release.

Example staging commands (not executed as release verification):

```sh
cmake --build build-03-release -j 4
mkdir -p dist
cp -R build-03-release/ReverieEnsemble.app dist/
"$(brew --prefix qtbase)/bin/macdeployqt" dist/ReverieEnsemble.app
# Next: inspect dependencies, collect notices and corresponding sources,
# verify user relinking, then sign/notarize and test on clean machines.
```

The presence of code licensed under Apache-2.0 does not impose a QuadraJ logo-display requirement on derivatives. Brand names/logos have their own trademark considerations; third-party licenses remain separate.
