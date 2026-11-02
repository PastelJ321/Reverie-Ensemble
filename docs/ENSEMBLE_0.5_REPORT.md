# Ensemble 0.5 — implementation and verification

Implementation/verification: 2026-11-02; review handoff: 2026-11-03, Asia/Seoul. Development PR, not GarageBand/Logic parity or a certified binary release.

## 1. Baseline and review

Same existing checkout, `codex/ensemble-0.5`, based on `codex/ensemble-0.4` at `3e3d229572d97dff88a1474d253e3e3a565407b4`. PRs #1/#2/#3 were verified open/unmerged. Baseline 0.4 Release CTest: 8/8. No existing PR was merged, closed, deleted or force-pushed. The 0.5 PR is stacked on 0.4.

## 2. Main files and architecture

`core/Plugin.*`, `plugin_scan_main.cpp`, `ui/PluginBrowser.*` provide CLAP lifecycle, isolated discovery, registry, parameters and state. `Model.*`/`Serializer.cpp` add schema 5 and inserts while reading v1–v4. `Engine.*` keeps Reverie's scheduler/mixer/output and adds block inserts and CLAP transport. `Dsp.*`/`Effects.*` add opt-in synthesis/dynamics/EQ. `ProductionControls.*`, Mixer, SoundBrowser and Settings expose actual operations. Factory/Meta selection, acquisition, checksums and packaging are reproducible scripts. Qt catalog sources, QM rules and localization guides are separate from Korean translation work.

## 3. Actual factory instrument inventory

The new full factory component has **51 recorded source instrument entries**. These include explicitly named physical variants such as two tambourines; they are not claimed as 51 unrelated instrument families. Grouping numbered tambourines and the two snare collections conservatively leaves **49 source instrument kinds**, including explicitly identified percussion/sound effects. Individual oscillators/EQ variations are not counted as recorded instruments. Complete selected SFZ maps retain all required sample regions/velocities/articulations. The exact names, categories, sizes and patches are in `measurements/0.5/factory-inventory.json` and the committed factory manifest.

The unchanged legacy component retains 13 recorded instrument entries. Some recordings overlap with the new full source maps (e.g. flute/violin sources), so **64 raw source entries are not 64 unique recordings**. The built-in synth is one instrument with 14 patches; the synthetic drum engine is another with one kit. Installed user plugins are excluded from factory counts.

## 4. Actual factory patch count

**99** new recorded SFZ patches + **14** preserved compact recorded patches + **14** built-in synth patches + **1** synthetic drum kit = **128 selectable factory patches**. Six extended synth patches describe genuinely different synthesis configurations; they are not six new sampled instruments. Original library patch/articulation names remain. Recorded percussion/sound effects are identified as such, not renamed pianos/guitars. This edition still lacks dedicated felt/upright, solo viola and a complete new acoustic contrabass palette; legacy compact guitar/bass/cello content remains available.

## 5. Factory installation and artifacts

New component: **1,377,008,415 payload bytes**, **2,818 audio files**. Preserved compact components: **99,584,968 bytes / 85 WAVs**. Combined sound payload: **1,476,593,383 bytes (~1.48 GB decimal)**, excluding manifests/filesystem overhead, fonts, artwork, application and shared dependencies. This fits the requested practical 500 MB–1.5 GB range without thinning velocity layers or shortening attacks/tails.

The full local `.app` actually contains `Contents/Resources/FactoryLibrary`; first launch requires no sample download. CMake `REVERIE_REQUIRE_FACTORY=ON` fails if its factory source artifact/sfizz is unavailable. A source-only developer build may retain the compact fallback and displays a missing-factory notice. `fetch_factory05.py --download` obtains just the selected payload, not the 9.33 GB full collections. `package_factory05.py` verifies and makes a separate factory tar.gz/SHA-256 in `out/dist`; no large new audio is committed to Git and no public Release was uploaded.

The build remains developer-linked to installed Qt/sfizz/FluidSynth/codec libraries. It is not yet a signed/notarized clean-machine installer. See DISTRIBUTION.md.

## 6. Factory rights

VSCO/VCSL and the three Meta instrument directories are CC0-1.0. Meta is pinned at `251c0be57d5d1b3242b91f53e0cde62ab1cb42d9`; unrelated Meta SFX corpora are excluded. Original notices, maps, source URLs/Git blobs and SHA-256 are preserved. Mapping derivatives are explicitly identified. Engine/host source is Apache-2.0; dependencies retain BSD/MIT/LGPL/OFL terms. User music is not owned by Ensemble.

## 7. Optional libraries

Full VSCO 2 CE (24 source entries/75 patches) and VCSL (102/183) catalogs remain. Meta Piano/Nylon/Steel standalone catalogs are added; their recordings are **also provided by the factory component** and are not counted again as new sounds. Manager identifies factory inclusion and labels a separate-copy installation. The music browser hides redundant not-installed Meta download entries when that source is already in Factory.

## 8. New premium samples and compatibility

Meta Piano retains 352 original Ogg/Opus samples and four velocity layers. Nylon retains 348 original Ogg/Vorbis samples; Steel retains 375. The entire original files, licenses and maps accompany each collection. Piano is rendered through sfizz with dynamically linked libsndfile/Opus enabled. SHA/Git-blob checks confirm the sample bytes are unchanged.

sfizz 1.2.3 ignores `lochan`/`hichan`. Loading the unmodified guitar maps would incorrectly layer their multiple MIDI-channel masters. Therefore the playable Ensemble guitar editions retain **only the original channel-one automatic first-position mapping**, with its exact per-note/string samples, velocity splits/crossfades, Nylon keyswitches and per-string choke groups. Originals are retained for other samplers. The playable guitar derivative also disables original amplitude randomization so the measured edition is deterministic; velocity splits and sample bytes remain unchanged. Channel-per-string modes 2–7 are **not supported by this edition**; no full multichannel Meta compatibility is claimed.

Every new factory patch was rendered at three source root probes × three velocities: **99/99 audible**, finite audio. Fixed Meta gain comes from measured peak/RMS, capped at +12 dB and strong-note headroom, not compression/per-note normalization. Existing calibrated gains remain. These probes are not musician listening, all-key tuning/noise certification or a universal chord clipping guarantee.

Actual Qt HTTPS installation downloaded the complete **48,300,056-byte Meta Piano** collection, verified it, and rendered the installed Opus piano. A separate native UI take records/saves/reopens/exports Meta Piano offline.

Salamander was reviewed (CC BY 3.0, 16 layers/release/resonance) but not integrated. VCSL Keys is a lossless repack of overlapping VCSL keyboards, not counted as new recordings. Existing verified Karoryfer bass/guitar content remains; additional Karoryfer/SM/Muldjord/Big Rusty drum packs and licensed GM banks are not newly certified. No blanket CC0 assumption is made for Marie Ork or other exceptions. See library research notes.

## 9. CLAP hosting

Real installed bundle discovery, MIDI instruments, stereo processing, generic parameters, opaque state/project restore and offline WAV. A bounded scanner child isolates discovery crashes/timeouts. A deliberately aborting fixture verifies the UI survives. Shared module handles are cached (bounded to 128) and released at shutdown; this avoids repeated module-global loader registration and redundant binary initialization. Updating loaded plugin binaries requires restarting the host.

Lifecycle/state/activation are dispatched to the main thread; audio processing uses fixed blocks/events. Independent plugin assertions validate thread roles. Audio playback still happens in Ensemble's process: **runtime plugin crash containment is not provided**. See PLUGIN_CAPABILITIES.md for ports, events, extensions and limits.

## 10. Audio Units

Not implemented. No AU editor/state/render support is advertised. Existing system `.component` installations are not misrepresented as usable.

## 11. VST3

Not implemented. The current MIT SDK was reviewed as a future route, not linked or marketed as a working host.

## 12. Actual synth/plugin compatibility

Official **Surge XT 1.3.4 CLAP**: discovery, actual MIDI audio, parameters/state, Musical Typing, sustain, MIDI clip capture, project reopen and WAV. **Surge XT Effects**: actual load, parameters/state and insert/offline processing; unconnected auxiliary inputs receive silence. Dexed and Vital are unverified/unsupported through their AU/VST3 builds here. No third-party synth binaries, GPL source or Vital factory media are bundled.

## 13. Plugin state and editor scope

ID/path/vendor/version and up to 8 MiB opaque state are serialized; ordered inserts also save bypass/wet. Missing instruments are silent/recoverable and missing effects are bypassed with preparation warnings. A generic parameter editor commits on Apply and safely prepares a new state. Native plugin GUIs/preset browsers, automation recording and seamless held-note/FX-tail continuity during graph edits are not implemented. Host restart requests require preparation again; unsupported extensions are not advertised.

## 14. MIDI, rendering and compatibility

Native Cocoa/miniaudio: **isolated Surge XT three-note chord with sustain → MIDI clip → save/reopen → WAV**, and **isolated Meta Piano two-note take → collected media → reopen → WAV**. Other demo tracks were removed from these probes so their audio cannot mask a failed instrument. Automated real Qt key events/device buffers are the evidence; this was not a musician's performance or listening session.

Legacy First Light v1, Demo Composition v3, Typing Study v4 and the collected 0.4 sfizz Grand take produce byte-identical WAVs with 0.5. Additional legacy core/editor migration checks remain. CoreMIDI code/input and SoundFont paths are retained; physical external keyboard/pedal/hotplug testing was not available. MIDI takes remain single-arm linear recordings, not loop overdub/comping.

## 15. Production DSP and inserts

One opt-in extended synth: dual oscillators/detune, mono last-note priority, legato, portamento, ADSR, filter envelope, LFO/vibrato, velocity and simple FM. Old presets keep the original DSP path. New controls actually change DSP and save/restore.

Channel processing adds a stereo-linked compressor (threshold/ratio/attack/release/makeup), one RBJ parametric peaking EQ band, input/output gain and an instant peak safety limiter. These are foundations, not a professional multiband EQ/true-peak lookahead suite. Existing EQ/delay/reverb remain. Ordered CLAP inserts support reorder, bypass and wet/dry. Independent gain/bias plugin tests assert noncommutative order, reordering, exact bypass/wet behavior and state/thread roles.

Latency/tail are reported and bounded tails are included; **no plugin delay compensation, routed sidechains, buses/sends or automation**. Auxiliary output buses are discarded. State changes/graph preparation can release monitored notes and restart effect tails. Long-run callback/latency profiling remains work.

## 16. Korean technical foundation

Qt `QCoreApplication::translate`, QTranslator, LinguistTools lupdate/lrelease, external TS/QM packaging and saved English/한국어/system selection. Restart requirement is explicit; no project/audio/plugin/recorded data is reset. The source catalog is intentionally unfinished for Argon. A separate three-string Korean QM fixture verifies actual UTF-8 font/rendering and persistence. Runtime retranslation of every current widget and finished Korean wording are not claimed.

## 17. Argon handoff

Translate `translations/ensemble_ko_KR.ts`; follow LOCALIZATION_GUIDE.md and TRANSLATION_KEYS.md. C++/generated extraction declarations belong to implementation work; fixture TS is QA only. Work in a coordinated localization branch/PR based on 0.5, preserve contexts/placeholders/IDs, and review lupdate changes rather than regenerating translations in parallel.

## 18. Builds and automated checks

Debug/Release: **12/12 CTest**. Qt-free ASan+UBSan: **7/7**, including the instrumented independent insert plugin. External sfizz/FluidSynth/Surge binaries are not sanitizer-instrumented; no claim is made about their internals. Logs and measurements are retained. An initial concurrent Debug/Release UI run shared QA preferences/artifact paths and failed before creating the second take. The QA runner now isolates preferences and offscreen outputs by build; final suites are run separately. macOS/arm64 and Qt 6.12.0 were exercised; Intel/non-macOS certification was not performed.

## 19. Real audio evidence

Native isolated device outputs were nonzero for Surge and Meta; actual client/backend formats and peaks are in native logs. Device choice followed the system default; Bluetooth/headphone/speaker and physical round-trip latency are not inferred from the nominal buffer queue. Native project/WAV artifacts remain under `out/native05/`, with collected Meta `.media`. The factory archive remains under `out/dist/`. Keep the Meta project and its sibling media folder together. Randomized synth/sample features are not claimed to render identically across independent sessions; the specific legacy golden WAVs do.

## 20. Actual UI captures

`docs/screenshots/0.5`: full factory browsing, isolated plugin/Meta recording, generic plugin parameters, installed plugin scan, channel processing, library installation and Korean **fixture** layout. Native captures are Cocoa widget/device sessions; offscreen captures are labeled separately. The 0.4 asset system/design remains. Unknown instruments use small family icons and omit misleading large artwork rather than showing a marimba as handclaps. Native review caught/fixed a compact-detail overlap and stale Piano Roll horizontal scroll hiding a new take.

## 21. Explicit incomplete work

AU/VST3, native plugin GUI/preset browsers, PDC, runtime crash bridge, general timer/fd/threadpool host extensions, live binary reload, automation, buses/sends/sidechains, loop comping, human quality/tuning/noise review, long sessions, real MIDI hardware, clean-machine signed deployment and finished Korean translation. Guitar channel-per-string modes are not provided. Current snapshots can reprepare plugin instances on edits; seamless mixing/control continuity needs more work. Required RAM measurements in audits are engine buffer estimates, not total process bounds.

## 22. Remaining GarageBand/Logic gaps

First-launch playable content and a real third-party plugin path now exist. GarageBand/Logic still provide substantially more polished latency/device handling, plugin GUI/preset workflows, effect routing/PDC, automation, take management, content curation and release packaging. An audible render test does not establish equivalent musical quality.

## 23. Recommended 0.6 order

Native CLAP editor/live parameter routing and continuous graph edits; PDC and runtime plugin diagnostics/bridge; AU then VST3 complete workflows; physical MIDI/latency/long-session work; musician-led factory tuning/noise/drum curation and articulation UI; finished Argon Korean review; standalone signed/relinkable distribution; then buses/sends/automation and loop takes.
