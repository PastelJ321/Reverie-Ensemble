# Verification record — 2026-10-23 (Asia/Seoul)

Tested locally on macOS 26.6.2 (25G83), arm64, Xcode Apple Clang 21.0.0, CMake 4.4.3, Ninja 1.13.2 and Homebrew Qt 6.11.2. Results below describe executions performed in this development session; they are not certifications of professional-session reliability.

## Executed and passed

| Check | Result |
|---|---|
| Debug native Qt app + tools build | Passed with Clang / Ninja |
| Debug CTest `audio-core`, `editor-workflow` | 2/2 passed |
| Release native Qt app + tools build | Passed |
| Release CTest `audio-core`, `editor-workflow` | 2/2 passed |
| Qt-free core build/test | Passed |
| ASan + UBSan Debug core build/CTest | 1/1 passed; no sanitizer errors reported |
| Native Cocoa window startup, render and normal timed shutdown | Passed; app-owned screenshot inspected |
| Real miniaudio output-device startup and short playback | Passed; Debug probe progressed to frame 67,328 with peak 0.146456; Release probe frame 67,072 / peak 0.156243 |
| Offline demo WAV inspected by macOS `afinfo` | 48,000 Hz, 2 channels, Int16, 480,000 frames / 10 seconds, RIFF data offset 44 |
| Demo save → separate CLI load → render identity | `cmp` verified byte-identical WAV output |
| Synthetic Release callback CPU workload | 16 tracks / 256 held voices / 48 kHz / 256 frames; p50 0.590 ms, p99 0.975 ms, max 1.172 ms on this run |

The 256-frame deadline is 5.333 ms at 48 kHz. The benchmark ran 750 blocks; it is a CPU measurement under one local load and is **not** a device round-trip latency measurement or a long-session glitch test. Scheduler load and hardware configuration can change results.

The initial sandboxed native startup could not access macOS pasteboard/window services and exited with code 134. Repeating the native startup outside that restricted process sandbox succeeded. Offscreen Qt workflow tests ran normally inside the sandbox. This distinction is recorded so an execution-host restriction is not misreported as a passed launch.

Current generated local evidence is in ignored `out/`: `editor.png`, `First-Light.wav`, `First-Light-restored.wav`, `benchmark.txt`, `wav-info.txt`, native startup/audio probe logs and test logs. These are development artifacts, not external uploaded files.

## Automated coverage

`audio-core` does not open audio hardware. It verifies:

- fractional-tempo/sample/beat conversion, no output before the scheduled MIDI start, a nonzero sample immediately after the correct onset, and complete release after Note Off;
- multiple tracks, 20-note polyphony, all factory presets at 44.1 kHz, drum keys 36/38/42/46, volume/master, hard pan, Mute and Solo against rendered samples;
- seek into a held note and oscillator/envelope identity after replacing an unchanged graph;
- no C++ `new/delete` during instrumented callback graph adoption, rendering and seek;
- project/preset save/load, identical rendered audio after project round trip, content-collected WAV references and cached waveform bins; resaving/reopening after moving the external original, including Undo/Redo history;
- valid PCM16 WAV export, WAV import/playback, sampler output and final-frame interpolation;
- missing WAV references produce warnings and silence; corrupt JSON, invalid tempo/sample rate and failed validation leave existing saved project data intact;
- capture-ring/worker output frame count and resulting WAV header, explicit capture overflow detection;
- 600 publish/seek/play/collect operations from a producer while a separate audio consumer renders, under ASan/UBSan in the sanitizer run.

`editor-workflow` runs Qt Widgets with `QT_QPA_PLATFORM=offscreen` and real widget events. It creates a MIDI track and clip through the timeline, draws/moves/resizes a piano-roll note, moves/resizes a clip, changes velocity, applies Undo/Redo, changes mixer/preset/BPM, duplicates/deletes clips, saves/opens a project, imports WAV and exports WAV. It also injects an output initialization failure and verifies safe error reporting. These tests use actual project/core classes and file formats, not mock audio data or fake meter values.

## Reproduce

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
cmake --build build -j 4
ctest --test-dir build --output-on-failure

cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DREVERIE_BUILD_GUI=OFF -DREVERIE_SANITIZE=ON
cmake --build build-asan -j 4
ctest --test-dir build-asan --output-on-failure

build-release/reverie-benchmark
mkdir -p out
build-release/reverie-render --demo out/demo.wav
build-release/reverie-render --write-demo out/demo.reverie
build-release/reverie-render out/demo.reverie out/restored.wav
cmp out/demo.wav out/restored.wav
afinfo out/demo.wav

# Native desktop session required; this briefly plays the demo through the default output:
build-release/ReverieEnsemble.app/Contents/MacOS/ReverieEnsemble --audio-smoke-test
# Startup screenshot without opening an audio device:
build-release/ReverieEnsemble.app/Contents/MacOS/ReverieEnsemble \
  --no-audio --smoke-test --screenshot out/editor.png
```

`--audio-smoke-test` exits 0 only if the engine frame counter advances beyond 24,000 and the measured output peak is nonzero. It does not capture the physical speaker/loopback and does not prove that a human heard an undistorted signal. Normal startup does not automatically play audio.

## Not executed / manual release checks

- Actual microphone capture, macOS permission allow/deny/restricted/reset flows, input-device removal and resulting clip placement against an external time reference.
- Round-trip latency, monitoring, separate-device clock drift, hotplug recovery, no-device native workstation, 44.1 kHz physical output, negotiated hardware buffer size, long playback/recording and stress under competing CPU/disk loads.
- Full disk, partial filesystem failure, power-loss save durability, atomic save on Windows, deeply nested/adversarial JSON fuzzing and large media-library memory behavior.
- Intel macOS, universal binaries, Windows, Linux, accessibility review, all display scales/window sizes, notarized/quarantined clean-machine launch and LGPL library replacement in a staged release.
- SF2/FluidSynth or external plugin testing (not integrated).

Before publishing a binary, run a complete manual song session: new project → several MIDI clips/chords → presets → edit while playing → mix/pan/mute/solo → import WAV → record microphone → undo/redo → save/close/reopen → export → listen to and inspect the export. Record devices, sample rate/buffer, measured latency, glitches and all failures. Do not mark these checks complete from the automated results above.
