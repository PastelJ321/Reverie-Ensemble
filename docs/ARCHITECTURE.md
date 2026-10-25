# Reverie Audio Core 0.1

The engine is original C++20 code. There is no JUCE, Tracktion, Ardour, Qt Multimedia, or external DAW engine. Qt is used only by the desktop editor and the macOS permission bridge's UI event loop.

## Modules

`Model` owns Project → Track → Clip → Note value objects. IDs survive serialization, while duplication creates fresh IDs. Tracks are MIDI or audio. `Preset` describes a synth, synthetic drums or a pitched WAV sampler. `Sample` owns decoded stereo float PCM and precomputed waveform min/max bins; clips and undo snapshots share immutable samples. Audio media is referenced separately from preset parameters.

`TempoMap` converts quarter-note beats to rounded integer sample positions and back using double precision, the engine sample rate, and project BPM. 0.1 supports a single tempo and time signature. Displayed bar lengths account for the denominator; note/clip positions always remain quarter-note beats. Future tempo automation belongs behind this conversion layer.

`Synthesizer` provides 32 statically allocated voices per MIDI track, sample-token Note On/Off, velocity, linear ADSR, sine / polyBLEP saw / polyBLEP square / triangle oscillators, a one-pole low-pass filter and gain. Factory presets alter those parameters and harmonic color. Synthetic kick, snare and hats use oscillators/noise; sampler voices use root-note pitch ratio and linear interpolation. The lightweight `Instrument` and `AudioEffect` contracts separate sound generation from effects; no plugin host is implemented.

`AudioProcessingGraph` is prepared from a validated project on the control thread. It stores sorted absolute-sample events, a prebuilt note interval index, immutable audio regions, mixer settings and DSP voice state. Note Off sorts before Note On at equal times. Notes are clipped by the MIDI clip's visible end. Overlapping audio clips sum. Stereo balance keeps both channels unchanged at center and attenuates the opposite side toward hard pan. Solo is global, while an explicit mute still silences a soloed track.

`AudioEngine` contains transport atomics, pending graph exchange, a retirement SPSC ring and peak meters. `AudioDeviceManager` uses miniaudio's macOS Core Audio backend, requests stereo float at 48 kHz with a 256-frame target, and safely reports unavailable output/input. 44.1 kHz core rendering is tested, while GUI device configuration is currently fixed to 48 kHz. Device/backend conversion and negotiated hardware buffer/latency may differ from the requested period.

`AudioRecorder` owns a preallocated 262,144-frame stereo SPSC ring (~5.46 s at 48 kHz) and a disk worker. The capture callback only checks atomics and copies samples into the ring. The worker converts to PCM16, writes the WAV and finalizes its header. File-open failure is reported synchronously by a promise before capture starts. Overflow stops capture ingestion and preserves a valid partial take; disk failure is reported to the editor. Capture is stopped before draining/joining the worker. The main app remains editable/offline-capable after input denial.

`ProjectSerializer` parses/writes versioned JSON, validates IDs/ranges, collects content-hashed WAV media and swaps a temporary JSON into place after flush/close. Loading caches identical references, reports missing/corrupt/over-budget media, and restores the remaining project. `exportWav` uses an independent AudioEngine with the identical graph/synth/mixer code and streams blocks into a PCM16 encoder. Export is hardware-independent and runs on a worker thread in the GUI.

## Ownership and thread contract

```text
UI/control thread                 audio callback (one consumer)
Project edits
  → validate/prepare graph
  → pending.exchange(new)  ─────→ pending.exchange(nullptr)
     delete superseded pending      inherit unchanged voice state
                                     swap active graph
collect retired graphs       ←──── retired.push(old active)
  → delete on UI/control thread      render fixed-size scratch blocks
                                     write atomic meter/position values

capture callback                  disk worker
copy input to preallocated ring → consume/PCM16 encode/write WAV
```

The UI and audio thread cannot both own the same pending pointer: atomic exchange transfers ownership. Superseded, never-consumed pending graphs are deleted by the producer. Once activated, the callback owns a graph exclusively until it returns the pointer in the retirement ring. If the ring is full, the callback retains the current graph and delays the pending exchange. Collection runs on each edit and the 33 ms UI timer. No graph, vector, string or shared sample is destroyed in the callback. Engine destruction happens only after `ma_device_uninit` has stopped/joined output; its destructor then reclaims pending/active/retired graphs.

The callback uses fixed arrays and lock-free pointer/float/int64 atomics (compile-time assertions). It performs no project preparation, decoding, I/O, UI calls, logging, mutex waits, allocation or deallocation. C++ `new/delete` instrumentation tests snapshot adoption, seeking and rendering. This is not a proof that every possible OS/library operation has a hard real-time bound.

Schedule lookup uses binary search plus a prebuilt interval tree to recover at most 32 held voices after a seek. Seeks retrigger envelopes; they do not reconstruct elapsed oscillator/sample phase. Unchanged track sound/schedule fingerprints retain complete voice and event-cursor state across mixer edits. Per-track gains ramp to their new values over 128 samples. Master gain, instrument changes, clip edges and transport stop do not yet have general click-free transitions.

Audio region bounds, offsets and resampling ratios are prepared once. A fixed 256-frame scratch array mixes one track at a time, supporting callbacks larger than the requested period without resizing. Audio clips are checked per block; very large clip counts and worst-case 64-track/32-voice sessions have not been guaranteed within a real-time deadline. The benchmark measures a defined synthetic load.

## Editor and history

Qt Widgets owns the application window, library, transport, inspector, menus and progress dialogs. Timeline and piano roll use custom QPainter drawing and actual model IDs. Waveforms read cached bins; painting never decodes WAV. Drag gestures keep one before-state and publish at release. Simple commands publish immediately. During a gesture the visual preview can differ from the active graph until release; the callback never sees a partially edited project.

Undo/Redo retains up to 80 Project value snapshots; PCM is shared. Successful saving rebinds current and history media references to collected project files without replacing shared PCM; later saves therefore do not depend on an external original remaining at its old path. Selection is UI state and is not part of project/history persistence. Long operations take a project copy and run via `std::async`; a modal progress dialog keeps the Qt event loop and meters responsive while preventing competing edits. Recording locks project edits and inserts the finalized take as one undoable operation. Record media is retained even if an insertion is undone; no automatic destructive media cleanup is performed.

# 0.2 extension architecture

The existing core/model/graph/device/recorder/serializer modules are retained. Project schema 2 adds loop/metronome settings, zone-based presets and channel effects; loader migration accepts schema 1 without changing IDs, clip/note coordinates, colors, volume/pan or old media paths. The v1 First Light file remains unchanged as an automated fixture.

`Library` provides a control-thread weak sample cache, strict SFZ subset parser, versioned sound-pack metadata, integrity/path checks and staging-folder install. `Hash` streams SHA-256 without a Qt dependency. Presets own zones with shared immutable samples, and synth voices keep an integer zone index rather than pointers into an old graph's vector. Key/velocity mapping and sample start/end/loop/one-shot behavior execute on preallocated voices. Standalone/project serialization preserves all zone settings, provenance and instrument identity. Failed media references warn and stay silent.

`Effects` is a Qt-free per-track chain. It prepares delay/comb buffers outside rendering, then processes fixed storage. Two fixed tone bands split at 800 Hz; delay uses a precisely sized feedback ring; reverb uses four simple combs. During unchanged effect configuration graph adoption, buffer/vector ownership is swapped on the callback (constant-time, no allocation/free) so mixer edits retain tails; retired graph buffers are reclaimed on the control thread. Seek/loop clears effect state; profiling worst-case reset sizes remains a release requirement.

`AudioEngine` owns a separate pending/active audition graph and a bounded retirement path. The UI prepares its instrument and a short note. Audio consumes the prepared preview while the project transport is stopped or playing; project position/recording data is untouched. The latest prepared preview supersedes older pending previews; fixed 32 voices, duration cap, key release and early-key-up handling prevent unbounded stuck voices. Preview output is gain-limited and passes through master gain. Stop releases preview, and recording disables its controls. No audition graph is created/destroyed inside process().

Playback splits callback ranges at exact loop boundaries, resets scheduled note state and applies short boundary fades. Click synthesis follows the signature denominator and accents the numerator's first beat. Offline rendering constructs the same AudioEngine with monitoring/loop tools disabled, so metronome/audition do not leak into export and a loop cannot make export infinite. Release/effect tails extend export duration. `MidiFile` is a bounded type-0/1 PPQ parser; it preserves note timing/velocity and separates channels. It rejects unsupported tempo automation/SMPTE rather than importing them with silently wrong timing.

The Qt controller is separated from `Workspace`, `Theme`/`DesignTokens`, `SoundBrowser`, `InspectorPanel`, `MixerView`, `AudioEditor` and existing custom canvases. Large browser load/verify/install operations run through a worker with modal progress while UI meters remain responsive. The cache has one loader at a time; no UI edit races a pending library operation. Track-header and key/ruler overlays use viewport coordinates while musical data stays in project coordinates. Switching modes keeps the arrangement/editor widgets alive.

0.2 retains the 0.1 caveats described above: manual hardware latency/glitch/permission testing is still required, full sample-phase reconstruction and universal click-free transport are not implemented, and additional external instrument backends must prove their callback behavior before integration.

## 0.3 extensions

The C++/Qt boundary remains unchanged. Preset adds stable family/patch/articulation metadata, calibration version/linear gains and an opt-in cubic interpolation flag. Neutral defaults preserve old projects; only explicit new library selection reads factory calibration. SampleZone seq_length/seq_position uses fixed per-key counters and existing preallocated voices. No sample decoding or reference destruction is added to the callback. The uncalibrated synth accumulation branch preserves the original floating-point contraction, verified against old executable output.

LibraryInstrument is the legacy pack-entry name for a **patch**; instrumentId groups entries, patchId selects a sound, and SoundPack owns distribution/provenance. For compatibility pack.json still calls its entry array instruments. SoundBrowser displays unique instrument identities and a separate patch list; one recorded guitar's Natural/Staccato variants are counted as one instrument. Synth and acoustic assets remain distinct.

InstrumentArt provides original scalable QPainter silhouettes and a separate detailed rendering mode; InstrumentEditor exposes only existing cutoff/envelope/reverb DSP. ExportDialog and SettingsDialog isolate application-wide workflows from Editor/Inspector. Timeline keeps world-space musical coordinates while fixed headers use viewport offsets; seeking shows pending sample position immediately and loop edits are Undo transactions.

Export.h/Export.cpp define the hardware-free configurable renderer. It snapshots/validates a project, trims source events to the requested end, seeks the common AudioEngine without monitoring/loop tools or output clipping, spools bounded float blocks off the UI/audio threads, measures peak/RMS, then writes explicit little-endian WAV with optional normalization/TPDF. Float and PCM policies differ intentionally. Callback and project sample positions use the requested export rate. Temp files live beside the target; cancel/error removes only those files, successful close/rename replaces the target. Disk usage can include float staging plus the final WAV. The UI worker is joined before its snapshot/atomics are destroyed, including application shutdown.

Device changes stop/join capture and playback before AudioEngine::setSampleRateStopped clears graphs and republishes the project at the new client rate. Settings reports miniaudio's internal backend rate/format/period separately from client requests. Sample-rate conversion between client and backend is miniaudio's responsibility. Reported period duration is not physical round-trip latency. Device persistence uses names, with system-default fallback if unavailable. Automatic hotplug recovery remains unverified.

## 0.4 performance and external instruments

See [the implementation report](ENSEMBLE_0.4_REPORT.md) for the final architecture. `PreparedTrack` keeps the legacy DSP fast path and adds a prepared `InstrumentBackend` for sfizz/FluidSynth. Both paths share the event dispatcher, mix/effect/transport graph and offline renderer. New graphs load on workers; the callback only adopts/retires prepared ownership. The separate live graph accepts a bounded UI→audio SPSC queue while transport may be stopped. CoreMIDI callbacks enqueue bytes/messages into a separate bounded queue, drained every 4 ms; their mutex never enters audio rendering. Controller events and sustain-held spans are indexed for seeking. Offline sfizz explicitly uses freewheeling with the same sample quality setting.

Schema 4 stores backend path/bank/program, channels/controllers, arm/input state, clip mute and artwork metadata. SoundFont and plain-SFZ dependencies can be collected. Macro paths remain explicit external references. Missing external files/backends preserve project state and a silent, recoverable track with warnings. Original v1/v2 output was byte-compared against 0.3. This does not certify universal real-time safety of third-party binaries; their streaming code uses its own worker queues/try-locks and still needs stress/hardware profiling.
