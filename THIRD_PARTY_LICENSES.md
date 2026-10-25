# Third-party licenses

Copyright 2026 QuadraJ and contributors.

Apache-2.0 covers **original Reverie Ensemble code and original included factory/demo material**. It does not relicense Qt, miniaudio, nlohmann/json, system frameworks or Qt's dependencies. No GPL/AGPL DAW engine or commercial DAW instrument recordings are included. Version 0.2 includes a verified CC0 acoustic starter subset and OFL fonts, described below.

## Direct components

| Component | Version used in this build | Use | License |
|---|---|---|---|
| Qt Core, Gui, Widgets | 6.11.2 (minimum supported 6.4) | dynamically linked native UI | LGPL-3.0-only option |
| Qt Test | 6.11.2 | tests only; not linked into the app | LGPL-3.0-only option |
| Qt DBus and Cocoa platform integration | Qt build dependent; 6.11.2 here | indirect Qt Gui/platform dependencies | LGPL-3.0-only option; individual third-party notices retained |
| miniaudio | fixed tag 0.11.23 | device I/O, WAV decoder/encoder | MIT No Attribution (MIT-0) selected from upstream's dual choices |
| nlohmann/json | fixed tag v3.11.3 | Qt-independent project/preset JSON | MIT |
| Apple system SDK/frameworks | installed Xcode/macOS | system audio, windowing, mic permission | Apple SDK/platform terms; not relicensed or vendored |

miniaudio: <https://github.com/mackron/miniaudio/tree/0.11.23>, copyright 2025 David Reid. The unmodified header includes upstream/embedded notices, and the upstream license is at `third_party/miniaudio/LICENSE`.

nlohmann/json: <https://github.com/nlohmann/json/tree/v3.11.3>, copyright Niels Lohmann and contributors. The header retains its notice; full MIT text is at `third_party/licenses/nlohmann-json.txt`.

Qt: <https://code.qt.io/cgit/qt/qtbase.git/>, version tag `v6.11.2` in this environment. LGPLv3 and the GPLv3 text incorporated by it are at `third_party/licenses/Qt-LGPL-3.0.txt` and `Qt-GPL-3.0.txt`. These texts do not indicate that Reverie's own code is GPL. We select LGPL-compatible Qt Core/Gui/Widgets functionality and dynamically link Qt frameworks. No Qt Charts, Qt Virtual Keyboard, other GPL-only UI module or commercial-only module is linked. Build tools are separate tools, not linked runtime components.

## Qt binary distribution obligations

Follow the [Qt licensing documentation](https://doc.qt.io/qt-6/licensing.html), [LGPL obligations guidance](https://www.qt.io/development/open-source-lgpl-obligations) and the actual license texts for the exact build being distributed:

- Retain Qt copyright/attribution notices and include LGPLv3 plus its incorporated GPLv3 text. Inform users that Qt is used under LGPLv3 (About already does this).
- Preserve recipients' ability to modify/replace the LGPL libraries and relink/run the application with a compatible modified Qt. Do not forbid reverse engineering for debugging such modifications, and do not impose an EULA/DRM/signature rule that removes this right. Provide source/build/relink instructions for the application; dynamic linking is the default here. LGPL static linking requires additional relinkable materials and is outside this release configuration.
- Supply the exact corresponding source for distributed Qt/LGPL libraries, including modifications, relevant patches and build/configuration/install material, through a license-compliant distribution mechanism. For a downloadable binary, provide equivalent access to the matching sources as well. A generic upstream link alone is not the project's release-source process. Keep any applicable written offer/source availability obligations for the selected method.
- Where needed, include installation information so a user can install and run a relinked build. On macOS explain local ad-hoc re-signing after replacement (`codesign --force --deep --sign - My.app`), and do not require a QuadraJ signing key just to run a modified local build.
- Review indirect dependencies and their notices as well as Qt itself. Replacement must use compatible ABI/architecture/major version. `macdeployqt` collects libraries but does not by itself satisfy source/license obligations.

This repository supplies source, CMake build instructions and license texts. Local developer builds use the user's installed dynamic Qt. It does **not** publish a self-contained signed release or assert that running `macdeployqt` completes compliance. See `docs/DISTRIBUTION.md` for the release process and outstanding distribution work.

## Qt transitive components

The exact set depends on the Qt package/platform. In the tested Homebrew build, Qt Gui/Core also reference Qt DBus, GLib, FreeType, HarfBuzz, libpng, md4c, double-conversion, PCRE2, ICU, zstd and other dependencies. Examples of their licenses are GLib LGPL-2.1-or-later, FreeType FTL/GPL options (choose FTL when appropriate), HarfBuzz MIT-style, libpng's libpng license, md4c MIT, double-conversion BSD-3-Clause, PCRE2 BSD-3-Clause, ICU Unicode license and zstd BSD-3-Clause option. Treat this as an inventory to resolve against actual binaries, not a substitute for the exact notices. The installed Qt SPDX SBOM is at `<Qt-prefix>/share/qt/sbom/qtbase-<version>.spdx`; Homebrew formula source/build recipe and each installed dependency's licenses must accompany redistribution as appropriate. Qt's embedded components also have their own notices; consult [Qt's third-party inventory](https://doc.qt.io/qt-6/licenses-used-in-qt.html) and matching source.

`docs/DEPENDENCIES.sha256` records checksums for the vendored source/license files used by this project. The main project CMake does not download dependencies at configure/build time; miniaudio and JSON are vendored, and Qt is an explicit installed SDK dependency. The 0.4 external-engine additions are described below; no recorded SF2 bank is bundled.

## Version 0.2 sound and font assets

Community Essentials contains 35 unchanged VSCO 2 CE recordings under CC0-1.0 plus QuadraJ SFZ mappings. Credits: Versilian Studios / Sam Gossner, Ivy Audio / Simon Dalzell; sample cutting by Elan Hickler / Soundemote. Source revision, raw paths, URLs, exact sizes, root-mapping evidence and SHA-256 are retained in `assets/sounds/factory/pack.json`; the formal CC0 text is in the same folder. These recordings are not relicensed as Apache-2.0. See [Sound Library License Manifest](docs/SOUND_LIBRARY_LICENSE_MANIFEST.md) for the complete inventory and investigated-but-excluded libraries.

IBM Plex Sans KR Regular/Medium/SemiBold and IBM Plex Mono Regular are included unmodified under SIL OFL 1.1. Preserve `assets/fonts/OFL.txt` and `Mono-OFL.txt`, IBM copyright and reserved-font-name conditions. Fonts may be bundled with the app but not sold separately or relicensed as Apache-2.0. Original sibling resources were reused without alteration; the macOS bundle carries its own resources rather than a runtime path into another project.

Qt dynamic-link/source/relinking obligations above continue to apply. In historical 0.2, FluidSynth was not linked and SF2/SF3 hosting was disabled. The original limited SFZ parser remains a compatibility backend; 0.4 adds the separately licensed engines described below.

## 0.3 audio and illustration additions

Ensemble Essentials 1.0.0 contains 50 unchanged CC0 recordings from the individually verified Karoryfer Fashionbass / Black And Green Guitars repositories and VCSL. It is separate from the unchanged 35 VSCO 2 CE WAVs in Community Essentials 1.1.0. Exact repositories/revisions/raw paths and hashes are in assets/sounds/essentials03/pack.json; Fashionbass-CC0.txt, Guitar-CC0.txt, CC0-1.0.txt and original readmes accompany that pack. SFZ mappings are QuadraJ CC0 pack data; player/calibration code is Apache-2.0. See the expanded Sound Library License Manifest.

The source-code-rendered InstrumentArt silhouettes/detailed illustrations are original QuadraJ Apache-2.0 assets. CC0 audio permission was not used as a substitute for image permission: no third-party photos, Apple imagery, UI icons or preset artwork were copied. Existing IBM Plex OFL notices and Qt LGPL dynamic-link/source/replacement obligations continue unchanged.

Historical 0.3 did not link FluidSynth, libsndfile or new codec dependencies. Configurable WAV writing is original code; legacy miniaudio WAV decoding/encoding retains its existing license. Future encoders and SoundFonts require their own separate licensing and distribution review.

## Ensemble 0.4 external instruments and media

Original Reverie source remains Apache-2.0. New engine/sample integration does not relicense third-party code or the user's music. Full notices are in `third_party/licenses/instruments/` and copied into the app's Legal resources.

| Component | Pinned build / license | Integration |
|---|---|---|
| sfizz | 1.2.3, `4e70dc0bef53b41f2853ed46e26f5911114c92d0`, BSD-2-Clause | Shared library; original DSP/streaming engine, not the plugins/UI/JACK client |
| FluidSynth | 2.6.1, `71e85b2ca6bf48641ba7e2261d8f2640ae473773`, LGPL-2.1-or-later | Shared library built without Readline, device/network/unused effect integrations |
| GCEM | `012ae73c6d0a2cb09ffe86475f5c6fba3926e200`, Apache-2.0 | FluidSynth header dependency; archive SHA-256 pinned by its CMake module |
| Qt SVG / Network | Same Qt 6 license selection as Core/Gui/Widgets, LGPLv3 in this development build | Real small SVG asset rendering and HTTPS installer |
| VSCO 2 CE / VCSL | CC0-1.0; exact pinned source revisions and file URLs/hashes in `assets/sounds/catalog/` | Optional individual downloads; recordings retain source attribution even though CC0 does not require it |
| Generated instrument atlases | QuadraJ dedicates its rights under CC0-1.0; generated-output rights vary by jurisdiction | Original generated illustrations, source/prompts in `assets/artwork/README.md` |
| Small instrument SVGs / tone fixtures / example composition | QuadraJ Apache-2.0 | Original project assets, no Apple/commercial-DAW imagery or extracted samples |

The pinned sfizz build applies the recorded atomic_queue compiler spelling fix and an offline FilePool pending-queue fix (see `scripts/bootstrap_instruments.sh` and `third_party/patches/sfizz-1.2.3-freewheel.patch`). Original BSD/MIT notices remain in place.

The sfizz build retains Abseil (Apache-2.0), atomic_queue (MIT), ghc/filesystem (BSD-3-Clause), SIMDe (MIT), dr_libs (MIT-0), libaiff (MIT), WavPack (BSD), stb_vorbis (MIT option), st_audiofile (BSD-2-Clause), pugixml (MIT), KISS FFT (BSD-3-Clause), spline (BSD-3-Clause), tuning library (MIT), hiir (WTFPL), cpuid (BSD), JSL (MIT) and cephes' preserved BSD inclusion permission. Generated DSP headers declare their original authors/BSD-2-Clause; their notices and the Faust LGPL output exception/STK-4.3 text are retained separately. Optional JACK, plugins, test/benchmark clients and libsndfile are disabled in the sfizz build.

FluidSynth uses libsndfile for actual SF3 decoding. This local libsndfile dynamically depends on Ogg/Vorbis, FLAC, Opus, mpg123 and LAME. Their BSD/LGPL notices are preserved. The Homebrew FluidSynth binary examined during development also linked GPL Readline; **that is not the final application dependency**. The source bootstrap builds a Readline-free library, verified with `otool -L`. Do not silently substitute a system build and assume only its top-level LGPL applies.

A standalone distributor must ship the relevant license texts, exact corresponding LGPL library sources/build modifications and installation/relinking information, permit debugging modifications to those libraries, preserve replaceable dynamic linkage and check signing/runtime restrictions. Source/bootstrap instructions alone are not a claim that an eventual binary release has satisfied every source-offer obligation. No signed/notarized standalone binary or paid/private sample pack was published in this task. See `docs/DISTRIBUTION.md`.
