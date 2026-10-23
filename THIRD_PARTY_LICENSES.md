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

`docs/DEPENDENCIES.sha256` records checksums for the vendored source/license files used by this project. CMake does not download dependencies at configure/build time; miniaudio and JSON are vendored, and Qt is an explicit installed SDK dependency. No FluidSynth or SF2 asset is currently shipped.

## Version 0.2 sound and font assets

Community Essentials contains 35 unchanged VSCO 2 CE recordings under CC0-1.0 plus QuadraJ SFZ mappings. Credits: Versilian Studios / Sam Gossner, Ivy Audio / Simon Dalzell; sample cutting by Elan Hickler / Soundemote. Source revision, raw paths, URLs, exact sizes, root-mapping evidence and SHA-256 are retained in `assets/sounds/factory/pack.json`; the formal CC0 text is in the same folder. These recordings are not relicensed as Apache-2.0. See [Sound Library License Manifest](docs/SOUND_LIBRARY_LICENSE_MANIFEST.md) for the complete inventory and investigated-but-excluded libraries.

IBM Plex Sans KR Regular/Medium/SemiBold and IBM Plex Mono Regular are included unmodified under SIL OFL 1.1. Preserve `assets/fonts/OFL.txt` and `Mono-OFL.txt`, IBM copyright and reserved-font-name conditions. Fonts may be bundled with the app but not sold separately or relicensed as Apache-2.0. Original sibling resources were reused without alteration; the macOS bundle carries its own resources rather than a runtime path into another project.

Qt dynamic-link/source/relinking obligations above continue to apply. FluidSynth is **not linked or shipped**; SF2/SF3 hosting is not enabled. A future backend needs a fresh LGPL/build/transitive dependency review and callback safety validation. SFZ is a separate, limited mapping parser implemented by Reverie.
