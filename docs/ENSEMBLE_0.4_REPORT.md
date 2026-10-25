# Reverie Ensemble 0.4 — implementation and verification

2026-10-25, Asia/Seoul. This is an implemented development update, not a GarageBand-parity or production-distribution certification.

## 1. Branch and review

Submitted as [Draft PR #3](https://github.com/PastelJ321/Reverie-Ensemble/pull/3). Implementation commit: `8e9bb823f4f0a955901f11f7275129f433f16d23`.

`codex/ensemble-0.4`, based on `feature/ensemble-0.3` at `e9c58f4fa7f0bc3d571e3269a87b69a40f29aa6f`. PRs #1 and #2 were open/unmerged at baseline verification. Existing branches, commits and PRs were preserved. The 0.4 PR is stacked onto `feature/ensemble-0.3`; merge predecessors before retargeting.

## 2. Main implementation files

- `src/core/InstrumentBackend.*`, `SfzSupport.*`: external synthesis, capability reporting and dependency collection.
- `src/core/Engine.*`, `MidiRecording.h`: bounded performance queue, separate live graph, controllers, sustain restoration, frame-based takes.
- `src/core/Model.*`, `Serializer.cpp`: schema 4, backend/bank/program, note channels, controllers, arm/input state, clip mute and media collection.
- `src/platform/MidiInput.*`: CoreMIDI sources, packet decoding, hotplug notification and mock injection.
- `src/ui/SoundBrowser.*`, `SoundLibraryManager.*`, `InstrumentArt.*`: adaptive browsing, native installation, assets and cache.
- `src/ui/Performance*`, `Workspace.cpp`, `Editor.cpp`: virtual piano, typing, arm/count-in/recording and input routing.
- `src/ui/Timeline.cpp`, `PianoRoll.cpp`: real context commands and recording visualization.

## 3. Backend architecture

The existing Reverie scheduler, mixer, output, effects, recorder and exporters remain. The built-in synth/sampler retain their original fast path to preserve old render output. A common prepared-track event dispatcher handles both that path and `InstrumentBackend` instances. The latter provide tokenized note on/off, controllers, pitch bend, bounded block rendering, reset, offline mode, diagnostics and capabilities. Instrument configuration/state lives in the serializable Preset rather than opaque pointers.

External synths are loaded/prepared in workers. UI/control code publishes immutable prepared graphs; callback adoption swaps ownership and retires old graphs for control-thread reclamation. Live performance uses its own graph and 4,096-slot SPSC queue, independent of transport playback. Overflow panics safely. The CoreMIDI callback has a separate bounded input queue; its mutex is never acquired by the audio callback. The UI drains it every 4 ms.

## 4–6. sfizz, FluidSynth and format coverage

| Backend | Actual behavior | Limits |
|---|---|---|
| Built-in synth / sampler | Old rendering, calibration, fixed voices, CC64, multisamples and compact SFZ subset | CC1/pitch bend are preserved as MIDI data but do not modulate this legacy DSP |
| sfizz 1.2.3 | Real SFZ loading, stereo, velocity layers, keyswitches, round robin, release triggers, CC64/CC1, pitch bend, streaming, offline mode | SFZv2/ARIA compatibility is opcode-specific; one controller/keyswitch state per track, not multitimbral SFZ channels |
| FluidSynth 2.6.1 | Real SF2 and Vorbis SF3 loading/rendering, program inventory, bank/program selection, channel controllers, sustain and pitch bend | User supplies licensed SoundFonts; no new GM SoundFont bank is bundled |

sfizz is pinned at `4e70dc0bef53b41f2853ed46e26f5911114c92d0`; its pinned atomic_queue needs a narrow dependent-template spelling fix for current Apple Clang. A second narrow FilePool patch drains pending streaming requests before the offline wait: an initial concurrent-build export exposed a dispatcher race that could exhaust the preload before loading started. The live callback path is unchanged; the patch and repeat regression checks are retained. Bootstrap also avoids obsolete 32-bit ARM flags on Apple Silicon. FluidSynth is pinned at `71e85b2ca6bf48641ba7e2261d8f2640ae473773`. Its source build disables Readline, audio/MIDI device drivers, networking and unused effects/DLS integrations. The default application build prefers this audited local FluidSynth rather than silently adopting a system build with GPL Readline.

Original SF2/SF3 tone fixtures prove real parsing, rendering and bank 5/program 7 state. They are original test signals, not marketed recorded instruments. sfizz reports unsupported `group_label` metadata in 46 VSCO patches; this does not implement a group-label UI. VCSL audit reports no unknown opcodes. Neither fact is a claim that every possible SFZ extension works.

## 7–8. Full library inventories

| Optional source pack | Library instrument entries | SFZ patches | Audio files | Installed payload | License |
|---|---:|---:|---:|---:|---|
| VSCO 2 CE, original SFZ branch | 24 | 75 | 3,168 WAV | 3,178,497,498 bytes | CC0-1.0 |
| VCSL, original SFZ branch | 102 | 183 | 4,232 WAV + 6 MP3 + 18 Ogg | 6,155,672,931 bytes | CC0-1.0 |

Both complete pinned SFZ-branch payloads were actually downloaded and inventoried, not reduced to a few WAVs. **258/258 patches produced audio** in the audit (three source root probes × three velocities). These are 126 library instrument entries; overlapping instruments between the two collections are not claimed to be 126 unique physical recordings. The bundled 0.3 library remains 13 recorded instruments/14 patches/85 WAVs (~95 MiB), plus eight synth presets and synthetic drums. Test tones are excluded from these counts.

VCSL's SFZ branch accidentally nests Psaltery samples below the Italian harpsichord folder. The reproducible pack relocates those files to the paths required by the unchanged original mappings; sample bytes, mapping bytes, original URLs and hashes remain intact. Ogg/MP3 samples used by two legacy percussion patches are included. Every payload file has an SHA-256 and pinned Git-blob identity. Files such as source artwork/editor projects are not audio dependencies and are not included.

Catalogs include original patch names, instrument IDs, articulations, region counts, velocity metadata, audition roots, license/source, installation size and backend. Fixed per-patch calibration uses family targets and measured strong-note headroom, capped at +12 dB. It never compresses velocities or normalizes each note. Calibration and browser audition gain remain separate. Measured aggregate RMS spread falls from 37.58→23.80 dB (VSCO) and 34.08→23.44 dB (VCSL); brief percussion and naturally quiet dynamics are intentionally not forced to one level. Calibrated measured solo peaks are at most 0.60; this is not a universal chord clipping guarantee.

VCSL Keys, Salamander and additional Karoryfer collections were researched. They are **not** added as new complete packs in this update. VCSL itself supplies the available keyboard recordings; it is not relabeled as a separately validated VCSL Keys release. Salamander's ARIA-specific behavior and CC BY attribution require separate mapping/audio QA before a supported catalog is offered.

## 9. Sound Library Manager

Real native Qt HTTPS downloads from pinned public source URLs, per-file progress, explicit license/download consent, cancel, SHA-256/size verification, isolated staging, disk-space checks, atomic activation, update, remove and immediate browser refresh. Download bytes and installed payload are the same because files are transferred individually, not packed into a misleading ZIP-size estimate. Path traversal, duplicate paths and symlinked managed targets are rejected. Local unpacked packs remain supported. Active-project packs cannot be updated/removed while in use.

The explicit network test installed the **entire original VCSL Claps instrument**, rendered it, updated it, rejected a wrong hash while retaining the old install, cancelled a larger request without leaking staging, removed and reinstalled it. This tests the actual Qt HTTPS path; it is not a mocked HTTP response. Full 9.33 GB source acquisition and integrity/render audits were developer tools. A full 9.33 GB download through the GUI was not repeated. No unsolicited application downloads occur; no public release hosting was created. Interrupted downloads restart rather than resume. ZIP import is not exposed; there is no archive extraction attack surface.

## 10. Actual artwork assets

Two original image-generator outputs provide 28 raster instrument illustrations, committed under `assets/artwork/`, with prompts/provenance in that folder's README. They are illustrations, not photos of the exact sampled sessions. Other instruments use explicit family fallbacks; 102 unique VCSL-specific illustrations are not claimed. Original small SVG silhouettes are separate assets and use Qt SVG. Large images are never procedural QPainter instrument drawings. Image decode occurs off-thread; Retina-sized pixmaps and thumbnails are cached. User image/thumbnail URI fields, aspect ratio, license, author and source are persisted; missing images fall back safely.

## 11. Adaptive browser

Compact dock and persistent resizable expanded native floating browser (default 740 px), explicit toggle/Cmd+Shift+B, pin, category sidebar, instrument image list, separate patch column, details/source/license, actual phrase audition, Apply/Create Track, search, favorites, recents, installed filter, pack filter, drag/drop and double-click application. Choosing a patch preserves MIDI clips. The Inspector's competing synth preset selector was removed. Large-source preparation is a worker operation. Expanded width/size and mode persist; it can stay open across repeated choices. Compact layout remains usable at 1280×800 while the keyboard is open.

## 12–13. Virtual piano and Musical Typing

Dockable/floating 88-key MIDI 21–108 piano, mouse hold/release/glissando, real selected-track sound, visible notes, MIDI-number tooltips, octave, velocity, sustain and typing chords. `A W S E D F T G Y H U J K` plays chromatic C–C; `Z/X` changes octave, `C/V` changes velocity, `Tab` holds sustain. Auto-repeat is ignored. Text/number fields and modified application shortcuts do not play notes. Key-up, focus loss, deactivation, hiding, instrument changes, Stop and device loss release held notes. MIDI channel selection also routes virtual input correctly. GM drum hints/default octave are provided.

## 14–15. MIDI recording and external devices

One armed MIDI or Audio track at a time, explicit mode, count-in none/1/2 bars, signature-aware metronome, actual MIDI notes/channels/velocities and controller/pitch-bend events, frame-to-beat placement, live take visualization, held-note closure at Stop, one undoable clip, Piano Roll edit, save/reopen and WAV rendering. MIDI capture does not produce a WAV in place of notes. Instruments can be replaced after capture. Recording is linear; cycle playback is bypassed during a take, and overdub/loop-take comping is not implemented.

CoreMIDI discovers/selects sources, handles running status, Note On/Off, zero-velocity Note On, CC64, CC1 and pitch bend; source changes trigger safe note release. Mock-device injection exercises the shared recording/audio flow. **No physical external MIDI keyboard or pedal was connected.** Hardware hotplug, measured physical latency and long-session reliability are not certified. Input timestamps use the transport frame estimate and CoreMIDI host-time age; UI keyboard input resolves at audio-block boundaries, not a physical sample-accurate latency claim.

## 16. Context menus

Arrangement clip Open/Rename/Duplicate/Split/Copy/Paste/Mute/Delete/Properties; track Rename/Duplicate/Instrument/Color/Arm/Mute/Solo/Move/Delete; empty area Add MIDI/Audio/Create MIDI Clip/Paste/Import. MIDI splitting preserves notes across the split and clips note lengths. Piano Roll right-click opens Delete/Duplicate/Quantize/Velocity/Length/Transpose/Properties and never immediately deletes. Browser Preview/Apply/New Track/Favorite/Details/Source/Reveal/Manager use real data. Project edits participate in Undo/Redo. Unsupported commands are omitted or disabled; there is no fake command palette.

## 17–18. Compatibility and executed checks

- Debug CTest **8/8**, Release CTest **8/8**.
- Qt-free ASan+UBSan **4/4**, no reported findings. Third-party shared libraries were release builds, so this is not full sanitizer instrumentation of sfizz/FluidSynth internals.
- Actual SFZ/SF2/SF3 render, bank/program, sustain, polyphony, queue overflow/panic, controller capture, timestamp/duration/count-in/Stop, pedal-held seek restoration, source dependencies, schema restoration and offline output.
- Actual UI key events → chord + mock external MIDI → MIDI clip → collect/save/reopen → WAV; native context menus and source program selection.
- Native Cocoa + miniaudio output, **full sfizz Kawai Grand**, Musical Typing nine-note take with count-in/sustain/octave/velocity, portable save/reopen and WAV. Native probe peak approximately 0.047–0.050 after calibration. These were automated key events and output measurements, not a human musical performance/listening evaluation.
- Device requested 48 kHz float32 stereo / 256 frames; backend reported 44.1 kHz float32 stereo / 256×3. Reported queue duration 17.415 ms is not round-trip latency.
- First Light v1 and legacy v2 render byte-identically to the 0.3 executable. SHA-256 pairs are in measurements.

## 19. Screenshots and example artifacts

Actual Qt widget captures and native Cocoa Retina captures are in `docs/screenshots/0.4/`: Compact, Expanded, detail, Musical Typing, virtual piano, MIDI recording, Arrangement/Piano Roll menus, Library Manager and SoundFont selection. Offscreen menu/dialog captures are labeled as such; native captures are not HTML recreations.

`examples/Typing Study.reverie` / `.wav` reproduce the actual automated MIDI workflow using existing bundled sample bytes. The native full-library take is left at `out/native-performance04/Paper Study.reverie`, its collected `Paper Study.media` (~435 MiB) and `Paper Study.wav`. The large collected library is deliberately not committed to Git. Keep project and media folder together.

## 20. Remaining limitations

Offline output reserves literal SFZ `ampeg_release` (including ordinary includes, bounded to 120 seconds), at least 30 seconds for one-shot mappings, and a conservative 10 seconds for SoundFonts. This estimate is saved with the preset and tested with audible long-release output beyond the old two-second tail. Macro/CC-derived envelopes, unusually long one-shots and SoundFont releases beyond that hint can still require manual export range extension.

Arbitrary seek/split sample-phase and keyswitch-history chasing remain limited; complete-song rendering from the start was tested. No physical MIDI hardware/pedal QA, musician listening, hours-long sessions, clean-machine signed/notarized distribution, Intel or non-macOS UI certification. No loop overdub/comping, MPE, aftertouch, input latency compensation, custom typing layouts, plugin hosting or automatic drummer. Some external controls are defined by the source SFZ/SoundFont and legacy synth-only controls are disabled for them. Changing monitor preparation currently releases held performance notes. Live source selection favors safety over seamless sustained program changes.

Imported user presets/programs are retained in a local user-instrument registry and projects; original external library paths still need to remain available. Complex macro-based user SFZs can play through sfizz but cannot be safely collected by the plain dependency scanner; saving preserves their external reference and reports that portability depends on the external library. Managed catalogs reject unaudited macro paths. Source packs use streaming; buffer measurements do not bound total process RAM or guarantee real-time disk performance. No claim is made that every sound is audibly equal quality or every SFZ opcode is supported.

## 21–22. GarageBand gap and 0.5 priorities

The find → preview → choose → play → record → edit → save → render path now exists with actual engines and recorded libraries. GarageBand still has substantially more polished latency/device handling, built-in content curation, instrument-specific controls, sustained patch switching, audio recording ergonomics and creative tools.

0.5 priorities: physical MIDI/pedal/latency sessions and long-run callback profiling; richer user-bank management and controller editing; loop overdub/take management; seamless instrument swaps; per-instrument artwork coverage and musician-led library/audio QA; reproducible signed standalone LGPL replacement/source distribution; then AU/plugin hosting and broader editing tools.
