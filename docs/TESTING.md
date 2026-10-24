# Ensemble 0.3 verification - 2026-10-24 (Asia/Seoul)

Apple Silicon macOS, Apple Clang 21, CMake/Ninja and Homebrew shared Qt 6.11.2. Baseline: main f5c042d; PR #1 remained OPEN at head 10373219245cfa5d2f24be97b569b4d2f7a49dd2, so feature/ensemble-0.3 branches from that complete 0.2 state. Initial 0.2 CTest passed 4/4. Historical [0.1](archive/TESTING-0.1.md) and [0.2](archive/TESTING-0.2.md) records remain.

## Executed checks

| Check | Actual result |
|---|---|
| Debug native app/tools build and CTest | 6/6 passed |
| Release native app/tools build and CTest | 6/6 passed |
| Qt-free Debug ASan + UBSan build/CTest | 3/3 passed; no sanitizer findings |
| Legacy First Light v1 WAV vs prior output | Byte-identical using cmp |
| Legacy v2 recorded-sampler fixture vs 0.2 executable | Byte-identical using cmp |
| Factory level calibration | 23 patches/presets, 3 pitch roots x 3 velocities, source peak/RMS and chord peak measured |
| New export matrix | 18 combinations: 44.1/48/96 kHz x PCM16/24/float32 x mono/stereo; headers, decoded rate/frames and nonzero content checked |
| Export range/tail/normalize/dither/cancel/error | Passed; float headroom/PCM saturation and preserving an existing destination on cancellation checked |
| Recorded instruments | All 14 recorded patches / 13 instruments load, save with collected media, reopen and render |
| Native Cocoa/miniaudio demo output | 64,256 frames progressed, peak 0.0565002; actual six-track demo open/play/capture succeeded |
| Actual client/backend distinction | Requested/client 48 kHz float32 stereo / 256 frames; backend reported 44.1 kHz float32 stereo, 256 frames x 3 |
| Six-track Demo Composition | Saved/reopened export byte-identical in a separate CLI process; 44.1 kHz PCM24 and 96 kHz float32 decoded/inspected |
| Optional pack packaging | Local versioned ZIP, SHA-256 and download/install-size catalog generated and inventory checked; not publicly uploaded |
| Responsive UI captures | Actual 1440x900, 1280x800, Mixer, instrument/patch artwork, Export and Settings inspected; native Retina capture inspected |

[Release log](measurements/0.3/release-tests.txt), [sanitizer log](measurements/0.3/asan-tests.txt), [native report](measurements/0.3/native-audio.txt), [calibration details](INSTRUMENT_CALIBRATION.md), [demo metrics](measurements/0.3/demo-metrics.txt) and [synthetic CPU benchmark](measurements/0.3/benchmark.txt) are retained. Local build directories and generated WAVs are ignored.

The native backend's calculated 17.415 ms queued duration is **not physical round-trip latency**. The probe checks callback progression and a nonzero rendered peak; it does not record the speakers or certify audible quality. Native desktop access was required outside the restricted execution sandbox; offscreen Qt tests run within it.

## Test coverage

- `audio-core`, `features-02`, `editor-workflow`, `editor-02`: retain scheduler onset/Note Off, multiple tracks/voices, mixer controls, seek/graph handoff, real-time new/delete instrumentation, concurrent producer/consumer stress, recording ring/worker, corrupt/missing media, save/restore/collect, WAV import/render and real editing/Undo/Redo gestures.
- `features-03`: configurable WAV headers/rates/depths/channels/frames, release tail, explicit normalization, TPDF noise bounds, float over-range preservation, safe PCM saturation, cancellation/invalid destination cleanup, unchanged legacy calibration defaults, real recorded preset collection/restore/render, calibrated metadata and round-robin selection.
- `editor-03`: recorded guitar search/apply; distinct Natural/Staccato patch selection using different WAVs; visible artwork/silhouettes; precise ruler/playhead and Shift-loop gestures; numeric BBT/time entry; Export UI options and asynchronous file production; Audio Settings application/persistence in offline mode; exact responsive captures. No fake audio meters or mock project serializers are used.

The v1 bit comparison initially found forty 1-LSB differences among 960,000 samples caused by changed floating-point multiply/add contraction. Keeping the original uncalibrated accumulation branch restored byte identity. This is a verified local arm64 result, not a cross-CPU floating-point guarantee.

## Demo Composition

[Example project](../examples/Demo%20Composition.reverie): Concert Grand Piano, Electric Bass, Hollowbody Guitar, Acoustic Drums, Soft Pad and an Audio guitar bounce. At 108 BPM / 16 beats, 44.1 kHz / PCM24 stereo including tail: **656,600 frames, peak -11.0093 dBFS, RMS -32.0320 dBFS, 0 over-range samples**. The audio track is a rendered guitar WAV, not a claimed microphone take. Separate-process reopen and WAV comparison passed; native app opening/playback passed.

Browser/patch selection, piano editing, loop/seek, mixer and Undo/Redo are covered by real Qt events across the test suite. Project generation/mixing and export metrics are automated. A musician's complete hands-on listening/recording session has **not** been performed; these layers of evidence must not be conflated.

## Screenshots reviewed

- [Arrangement 1440x900](screenshots/0.3/arrangement-1440x900.png)
- [Arrangement 1280x800](screenshots/0.3/arrangement-1280x800.png)
- [Mixer](screenshots/0.3/mixer.png)
- [Export Audio](screenshots/0.3/export.png)
- [Settings](screenshots/0.3/settings.png)
- [Native Retina Demo Composition](screenshots/0.3/native-demo.png)

Images are captured from actual Qt widgets/application, not mockups. Inspection covered readable text, panel fit, artwork and patch identity, track-name elision, focus/contrast and compact layout. The native screen may constrain requested window height; exact logical sizes are verified in offscreen captures. Earlier screenshots remain under 0.2 for comparison.

## Reproduce

```sh
cmake -S . -B build-03-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
cmake --build build-03-release -j 4
ctest --test-dir build-03-release --output-on-failure
cmake -S . -B build-03 -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
cmake --build build-03 -j 4
ctest --test-dir build-03 --output-on-failure
cmake -S . -B build-03-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DREVERIE_BUILD_GUI=OFF -DREVERIE_SANITIZE=ON
cmake --build build-03-asan -j 3
ctest --test-dir build-03-asan --output-on-failure
build-03-release/reverie-demo03 assets/sounds out/demo03
build-03-release/reverie-demo03 --reopen 'out/demo03/Demo Composition.reverie' out/reopened.wav
cmp 'out/demo03/Demo Composition.wav' out/reopened.wav
build-03-release/reverie-calibrate assets/sounds out/levels.csv
build-03-release/reverie-benchmark
# Native desktop; briefly plays the included example through the actual device:
build-03-release/ReverieEnsemble.app/Contents/MacOS/ReverieEnsemble \
  --audio-smoke-test --size 1440x900 --screenshot out/native.png \
  'examples/Demo Composition.reverie'
```

`--audio-smoke-test` requires more than 24,000 project frames plus a nonzero rendered peak. Normal startup does not autoplay. `editor-03` generates its screenshots and WAV in ignored out/0.3.

## Not executed / release gates

- Actual microphone capture, permission allow/deny/reset, unplug/reconnect, long recordings, physical timing alignment, disk-full/power-loss tests and clean-machine device configuration.
- Physical round-trip latency, multiple-device clock drift, negotiated periods on other hardware, hours-long playback and hostile system loads. The 16-track/256-voice CPU benchmark is short and synthetic: p50 1.113 ms, p99 1.295 ms, max 1.817 ms at 48 kHz/256 frames against a 5.333 ms block deadline. It is not a device/glitch certification.
- Human listening evaluation for velocity-layer changes, pitch transitions, sample noise/tuning, calibration, loop transitions and stereo image. Engineering peak/RMS reductions do not establish perceptual equality.
- SF2/SF3/FluidSynth, external MIDI, pedal/controllers, monitoring/count-in/recording compensation, MP3/AAC, compressor, automation and clip fades/split: not implemented, hence not tested as supported features.
- Intel/universal/Windows/Linux, accessibility review, public online pack downloads, arbitrary archive extraction, signed/notarized standalone app and actual LGPL replacement/source distribution.

Before a binary release, record a physical instrument/microphone session, create/edit a whole song, listen to the rendered files on calibrated equipment, exercise failure/recovery and archive exact device/build/license evidence. See ROADMAP.md and GARAGEBAND_BENCHMARK.md for remaining product gaps.
