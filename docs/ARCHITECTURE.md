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
