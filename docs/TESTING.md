# Ensemble 0.5 verification — 2026-11-02

| Executed check | Result |
|---|---|
| 0.4 baseline | Release 8/8 |
| Debug / Release | 12/12 each |
| Qt-free ASan + UBSan | 7/7; independent insert fixture instrumented, external shared engines/plugins not instrumented |
| Full factory | 51 recorded source entries / 99 SFZ patches, 99/99 audible at three source root probes × three velocities |
| Premium HTTPS | Entire original Meta Piano collection, 48,300,056 bytes: download/integrity/Opus render passed |
| Actual plugin | Surge XT 1.3.4 instrument and Surge XT Effects: discovery, MIDI, parameters/state/project and offline processing |
| Insert semantics | Independent gain/bias fixture: order, reorder, bypass, wet/dry, parameter/state and main/audio thread assertions |
| Scanner containment | An intentionally aborting plugin terminates the scanner child; parent UI continues |
| Native Cocoa/device | Isolated Surge chord/sustain and Meta Piano takes → real MIDI clips → save/reopen → WAV |
| Localization | Production source extraction/QM build; independent Korean QM/UTF-8/layout fixture; language preference leaves project/plugin state intact |
| Golden output | v1 First Light, v3 Demo Composition and v4 Typing Study/full sfizz Grand match prior WAV bytes |

Measurements/logs live in [measurements/0.5](measurements/0.5/). Full scope and limitations: [23-part report](ENSEMBLE_0.5_REPORT.md). Source audio hashes and patches are retained in the factory manifest. Automated finite/audible output is separate from tuning/noise/listening certification.

## Reproduce

```sh
cmake -S . -B build-05 -DCMAKE_BUILD_TYPE=Debug
cmake --build build-05 -j 5
ctest --test-dir build-05 --output-on-failure
cmake -S . -B build-05-asan -DREVERIE_BUILD_GUI=OFF -DREVERIE_SANITIZE=ON
cmake --build build-05-asan -j 5
ctest --test-dir build-05-asan --output-on-failure

# Requires separate official Surge XT 1.3.4 CLAP bundles in out/surge05:
build-05/reverie-plugin05-tests 'out/surge05/Surge XT.clap'
# Native desktop; briefly plays through the real device:
build-05/reverie-ui05-tests --native
# Explicit real network acquisition via the application installer:
QT_QPA_PLATFORM=offscreen build-05/reverie-premium05-tests
# Complete offline inventory/render audit:
build-05-release/reverie-library-audit out/factory05 out/factory05-audit.csv --verify
```

The actual Surge tests are registered only when their separate official QA bundles exist; a dependency-free checkout must not report them as executed. The independent gain/bias insert test does not substitute for the recorded real Surge verification. `out/native05` is reserved for Cocoa captures/takes; offscreen tests write `out/ui05/<build-directory>` with independent INI preferences.

## Actual captures

Native factory/browser, plugin/MIDI/Meta take, plugin parameter editor, scanner, channel processing and Korean **test fixture** screenshots are under `screenshots/0.5`. The fixture is not a finished shipping Korean translation. Source assets are illustrations; application screenshots are actual Qt widgets, not generated mockups. The 1280×800 native workspace and preserved offscreen 1440×900 coverage are distinguished in the report.

## Known limits

No physical MIDI keyboard/pedal/hotplug sessions, human listening, hours-long stress, physical round-trip latency, AU/VST3/native plugin GUIs, PDC, runtime crash isolation, full multichannel Meta guitar modes, signed clean-machine distribution or finished Korean translation certification. See [plugin matrix](PLUGIN_CAPABILITIES.md) and [Argon handoff](LOCALIZATION_GUIDE.md). Previous test records are archived separately.
