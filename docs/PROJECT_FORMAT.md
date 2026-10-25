# Project format 4 — 0.4

Versions 1–3 remain readable. New files use version 4; older applications should reject that version explicitly. Legacy instrument kind values 0/1/2 retain their exact meanings; 3 is sfizz and 4 is FluidSynth SoundFont. New fields default neutrally on old projects.

Preset adds `backendPath`, `renderTailSeconds`, `soundFontBank`, `soundFontProgram` and artwork URI/thumbnail/license/author/aspect metadata. Track adds `armed`, `midiInput` and `midiChannel` (-1 all, otherwise zero-based). Note adds zero-based channel. Clip adds `mute` and controller events (`beat`, `channel`, `controller`, `value`); controller 128 represents 14-bit pitch bend. Timing remains quarter-note beats converted from captured transport frames.

Save collects SoundFonts with their actual extensions and plain-SFZ sample/include dependencies under content-addressed media folders, preserving relative mapping paths. Macro-based/uncollectable SFZs keep an explicit external reference; the UI reports that portability depends on retaining that library. Missing external instruments preserve references and warn rather than replacing the patch with a synth. Undo snapshots share sample PCM as before.

Keep the `.reverie` file and sibling `.media` folder together. Source libraries, calibrated gains and bank/program state are not inferred again on reopen; recorded MIDI can be reassigned to another instrument without changing notes.

---

## Historical format documentation

# Project and instrument formats

A saved project consists of:

```text
My Song.reverie             UTF-8 JSON
My Song.media/
  <FNV-1a-content-hash>.wav WAV copies from imported/sampler sources
  take-<timestamp>.wav      original local recordings
```

Both items must be moved together. Save As collects into a new media folder. Only referenced WAVs are copied. The serializer returns a Project with absolute references to collected files; the editor applies these references to current/Undo/Redo state while keeping decoded PCM shared. Subsequent saves prefer those durable model references over the PCM cache's original decode path. Recording/source files are not automatically deleted by undo, clip deletion, or saving. FNV-1a is used for stable local naming, not cryptographic integrity/authentication; a maliciously constructed collision is outside the current media collector's guarantees.

The JSON header uses `"format": "Reverie Ensemble"`. Legacy 0.1 files use schema `version: 1`; 0.2 wrote schema `version: 2`; current 0.3 writes `version: 3` and migrates v1/v2 on read. Product version and schema version are distinct. Unknown schema versions fail safely without replacing the active project. Current required top-level fields are name, bpm, numerator, denominator, master and tracks.

Track fields: id (positive persistent integer), name, kind (0 MIDI / 1 audio), ARGB color, mute, solo, linear volume (0–2), pan (-1–1), preset and clips. Current limit: 64 tracks.

Clip fields: id, name, beat (absolute quarter-note beats), length (quarter-note beats), sourceOffset (seconds into the unchanged source), media (WAV path), notes. Notes hold id, beat (relative to clip), duration (beats), key (0–127), velocity (0–1). Hidden notes after clip trim stay in the model; playback schedules only the visible intersection. Audio trim never overwrites the source. Overlapping clips sum.

Preset fields: name, kind (0 synth / 1 synthetic drums / 2 WAV sampler), wave (0 sine / 1 saw / 2 square / 3 triangle), attack, decay, sustain, release, cutoff, gain, color, rootNote, sample, source, license. Envelopes use seconds; cutoff is Hz. Sample ownership and waveform caches are reconstructed from WAV; PCM is not embedded in JSON.

Instrument preset files have `"format": "Reverie Instrument", "version": 1, "preset": {...}`. They contain parameter data and a separate sample file reference. Project saves collect those samples and store paths relative to the project directory. Standalone preset saves currently keep the sample reference as supplied (usually absolute); copy that WAV and update the JSON reference when distributing a sampler preset. `source` and `license` are independent strings that may contain a source URL and SPDX license or permission note. User-supplied files are explicitly marked as requiring user-managed redistribution permission. No license is inferred merely from importing a file.

Load validation rejects duplicate/invalid IDs, invalid numeric ranges, non-finite numbers, unsupported kinds/signatures and oversized projects. Project JSON is limited to 64 MiB, total notes to 200,000, clips to 20,000; preset JSON is limited to 1 MiB. WAV media is limited to 20 minutes / 256 MiB decoded PCM per sample and 512 MiB in the project loader's unique cache. Missing/corrupt/over-budget media retains its reference, emits a warning, and plays silently. Save copies media before replacing the JSON; an unsuccessful media copy or JSON write leaves the previous JSON intact. Temporary media/orphaned files may remain after an interruption.

Save uses an adjacent temporary file and filesystem rename, verified on macOS. It is atomic with respect to ordinary reader observation; it is not fsync-based crash/power-loss durability. Windows replace-existing semantics and paths need platform validation before declaring support. External paths can be referenced explicitly; loading does not download anything or execute code.

## Schema 2 (Ensemble 0.2)

Writes use `version: 2`. Reads accept 1 or 2 and migrate to in-memory version 2. Missing new fields use neutral defaults; original IDs and old sound parameters remain. The original v1 example is intentionally preserved and tested.

Project additions: `loopEnabled`, `loopStart`, `loopEnd` (quarter-note beats), `metronome`, `clickLevel`. Export ignores monitoring/loop state. Track addition `effects` stores enabled, lowDb/highDb, delayMix/delaySeconds/feedback/reverbMix. Neutral effects preserve the existing signal path.

Preset additions: instrumentId, category, tags and zones. Zone fields: path, lowKey/highKey, lowVelocity/highVelocity, rootKey, start/end/loopStart/loopEnd in source frames (exclusive ends), loop, loopContinuous, oneShot and linear gain. WAV PCM and waveform caches are not embedded. Project saving collects every zone path, rebases current/history paths and preserves identity/provenance; loading warns on missing media and continues safely. SFZ mappings are embedded into this schema, so their original SFZ file is not a runtime requirement after save. This is distinct from SF2/SF3, which are unsupported.

## Schema 3 (Ensemble 0.3)

Current writes use version 3; reads accept 1/2/3 and migrate in memory. The older sections describe historical schemas. New preset fields: family, patchId, articulation, calibrationVersion (default 0), calibrationGain/auditionGain (default 1), cubicInterpolation (default false). New zone fields: sequenceLength/sequencePosition, both default 1. All are embedded and preserved on save, including collected WAV references.

Calibration is never looked up or reapplied when opening a project. An explicit new factory-patch selection adopts its current metadata; old presets remain uncalibrated/linear. This prevents a library update from changing an existing mix. Standalone instrument schema remains version 1 with optional neutral-default fields; an old application may not reproduce new optional DSP behavior, so forward compatibility is not promised.

The original First Light v1 and tests/fixtures/legacy-v2.reverie exercise old synth and recorded-sampler mixes. Native arm64 exports were compared byte-for-byte against the prior executables. New format migration is covered by project/preset round trips; platform-specific floating-point identity is not claimed on untested CPUs.
