# Ensemble 0.4 verification — 2026-10-25, Asia/Seoul

## Executed results

| Check | Actual result |
|---|---|
| Debug build / CTest | 8/8 passed |
| Release build / CTest | 8/8 passed |
| Qt-free ASan + UBSan | 4/4 passed, no reported findings; external shared engines not sanitizer-instrumented |
| VSCO full source pack | 24 instrument entries, 75/75 patches audible; 3,168 WAVs, 3,178,497,498 bytes |
| VCSL full source pack | 102 entries, 183/183 patches audible; 4,256 audio files, 6,155,672,931 bytes |
| Real SF2 / SF3 | Original valid SF2/Vorbis-SF3 fixtures rendered; bank/program, controller, sustain, reset, project restore/export verified |
| SFZ offline release | Included 4-second release survives collection/reopen and remains audible beyond the prior two-second export tail; offline queue patch passed 20 repeated runs |
| Real pitch bend | Rendered zero-crossing frequency changes for sfizz/SF2/SF3, measured in correct offline streaming mode |
| Recording | Native key events and mock external messages → frame-aligned notes/channels/controllers → MIDI clip → save/reopen → WAV |
| MIDI safety | Chords, key repeat suppression, text focus, focus panic, Stop, held-note closure, queue overflow, channel routing and pedal-held seek restoration |
| Persistent user banks | SoundFont programs survive browser/app construction through the local registry |
| Native HTTPS installer | Complete 1,243,745-byte original VCSL Claps instrument + provenance; render, non-GM audition, update, bad hash rollback, cancel cleanup, remove/reinstall passed |
| Native Cocoa/device | Full sfizz Kawai Grand Musical Typing, nine notes with count-in/sustain/octave/velocity; nonzero actual device output and portable save/reopen/export |
| Legacy renders | v1 First Light and v2 fixture byte-identical to the 0.3 executable |
| Example reproducibility | Committed Typing Study project renders byte-identically to its committed WAV |

The two libraries were fetched with explicit developer download consent and pinned Git-blob/SHA-256 checks. C++ inventory/dependency verification was also executed. The GUI's actual HTTPS pipeline was exercised with a complete small instrument, not a mocked server; a second full 9.33 GB GUI transfer was not performed.

Native device: requested client 48 kHz float32 stereo/256; reported backend 44.1 kHz float32 stereo, 256 frames × 3. Nominal queue duration 17.415 ms is not measured round-trip latency. Full Grand native probe peak was approximately 0.047–0.050 after static family calibration. No physical loopback or human listening certification is implied.

## Evidence

Logs, library CSVs, pinned submodules, final linkage and summary metrics are under [measurements/0.4](measurements/0.4/). Raw and calibrated CSVs preserve per-patch audio/unsupported-opcode evidence; `group_label` is ignored in 46 VSCO mappings. The complete scope and limitations are in [the 22-part report](ENSEMBLE_0.4_REPORT.md).

## Actual screenshots inspected

| Screen | Capture |
|---|---|
| Compact 1280×800 | [Qt widget capture](screenshots/0.4/compact-1280x800.png) |
| Expanded three-column browser | [Native Cocoa](screenshots/0.4/native-expanded-browser.png) |
| Instrument detail | [Qt widget capture](screenshots/0.4/instrument-detail.png) |
| Musical Typing | [Qt widget capture](screenshots/0.4/musical-typing.png) |
| Virtual piano | [Actual custom Qt widget](screenshots/0.4/virtual-piano.png) |
| MIDI recording | [Native Cocoa](screenshots/0.4/native-midi-recording.png) |
| Arrangement context menu | [Actual Qt popup, offscreen platform](screenshots/0.4/arrangement-context-menu.png) |
| Piano Roll context menu | [Actual Qt popup, offscreen platform](screenshots/0.4/piano-context-menu.png) |
| Sound Library Manager | [Actual dialog](screenshots/0.4/sound-library-manager.png), [successful HTTPS installation](screenshots/0.4/installed-network-instrument.png) |
| SoundFont program selection | [Actual Qt input dialog, offscreen platform](screenshots/0.4/soundfont-selection.png) |
| Full native workspace | [native wide window](screenshots/0.4/native-keyboard-wide.png), [1280×800](screenshots/0.4/native-keyboard-1280x800.png) |

These are widget/application captures, not browser mockups or generated screenshots. Native captures include Retina pixel density. macOS constrained the requested 1440×900 native window to 1440×824 content on this desktop; [the separate offscreen widget layout is exactly 1440×900](screenshots/0.4/workspace-1440x900.png). Artwork itself is separately identified as original generated illustrations, with vector icons for small headers.

## Reproduce

```sh
sh scripts/bootstrap_instruments.sh
cmake -S . -B build-04-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-04-release -j 4
ctest --test-dir build-04-release --output-on-failure

cmake -S . -B build-04-asan -DREVERIE_BUILD_GUI=OFF -DREVERIE_SANITIZE=ON
cmake --build build-04-asan -j 4
ctest --test-dir build-04-asan --output-on-failure

# Explicit ~9.33 GB source download:
python3 scripts/build_community_catalog.py --download
python3 scripts/enrich_community_catalog.py
build-04-release/reverie-library-audit out/community04/vsco out/vsco.csv --verify
build-04-release/reverie-library-audit out/community04/vcsl out/vcsl.csv --verify

# Explicit small real-network integration, writes only under out/network04:
QT_QPA_PLATFORM=offscreen build-04-release/reverie-library-install04-tests
# Native desktop, briefly plays the full sfizz Grand through the real output:
build-04-release/reverie-native04-probe
```

## Practical limits

No physical MIDI keyboard/pedal, musician listening, long sessions, measured physical latency, clean-machine signing/notarization/LGPL replacement, Intel/non-macOS GUI certification. Current MIDI takes are linear, not loop overdub/comping. Complex macro SFZ collection remains external. sfizz has one controller/keyswitch state per track; SoundFont channels remain distinct. Monitor preparation changes favor note safety over seamless held-note continuity. See [0.5 priorities](ROADMAP.md). The [0.3 test record](archive/TESTING-0.3.md) is archived separately.
