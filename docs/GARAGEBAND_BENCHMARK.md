# GarageBand benchmark — Ensemble 0.3

Evaluated 2026-10-24 against Apple’s published GarageBand for Mac guide (10.4.14 as shown by that guide). This is a source-based feature comparison plus Ensemble’s own tests, **not** a hands-on comparative listening or usability study. No Apple audio, images, presets or UI assets are included.

Status applies to the whole category: **Complete** = all listed scope implemented and tested; **Partial** = a useful subset with explicit gaps; **Missing** = no implementation; **Not Yet Verified** = cannot substantiate behavior with available evidence. No overall GarageBand parity claim is made.

| Area | Apple documented capability | Ensemble 0.3 status and practical gap |
|---|---|---|
| Instruments / patches | Instrument patches and Smart Controls [1] | **Partial** — 13 recorded instruments / 14 recorded patches plus 9 synth/drum presets. Guitar Natural/Staccato are separate recorded patches of one instrument. Limited pitch/velocity coverage, no electric piano/acoustic guitar/amp models. |
| Sound library | Patches, sound packs, Apple Loops [1] | **Partial** — illustrated instrument/patch browser, search, favorites/recent, verified local packs. No comparable breadth, integrated online catalog or loop collection. |
| Recording | Audio recording and input monitoring [1] | **Partial** — existing WAV recorder/worker retained; hardware capture/permission/hotplug **Not Yet Verified** this session. Monitoring, count-in and latency compensation missing. |
| MIDI editing | Piano Roll, quantization, notation [1] | **Partial** — notes, velocity, selection, audition, quantize, Undo/Redo; no notation, pedal/CC editing or MIDI capture. |
| Audio editing | Region split, flex markers and pitch correction [1] | **Partial** — cached waveforms, move/trim/source offset. No split/fades/flex/pitch correction. |
| Drums / patterns | Drummer and percussion tracks [1] | **Partial** — recorded kick/snare/closed and open hats/toms, round-robin, existing synthetic drums. No Drummer, pattern browser or automatic accompaniment. |
| Effects / mixing | EQ, plug-ins, amps and pedals [1] | **Partial** — stereo meters, faders, pan, mute/solo, two-band tone EQ, delay and simple reverb. No compressor, parametric EQ, sends, amp/cabinet or plug-in hosting. |
| Automation | Track and effect automation points [1] | **Missing** — no automation lanes or controller playback. |
| Export / sharing | Disk export, MP3 quality, cycle/selection range [2] | **Partial** — complete WAV 16/24 PCM and float32 matrix, mono/stereo, 44.1/48/96 kHz, explicit normalize/dither, progress/cancel. MP3/AAC/AIFF/FLAC and service sharing missing. |
| Hardware / MIDI | Separate input/output device selection [3] | **Partial** — requested vs reported audio format/period, device selection/refresh and test tone. External MIDI input and physical latency/long-session verification missing. |
| Project management | Self-contained .band project packages [4] | **Partial** — versioned .reverie plus collected media, v1/v2 migration and atomic project replacement. No autosave, crash recovery, alternate takes or project-package UI. |
| Ease of use | Integrated production workflows [1] | **Partial** — Paper & Graphite, illustrated browser, family controls, draggable playhead/loop, direct position entry, dedicated Export/Settings. New-user, accessibility and musician listening studies **Not Yet Verified**. |

The largest remaining gaps are library expressiveness, MIDI/recording hardware workflow, automation, deeper audio editing and mature dynamics/effects. Greater screenshot polish or preset counts do not resolve these gaps. See [0.4 priorities](ROADMAP.md), [test evidence](TESTING.md) and [level measurements](INSTRUMENT_CALIBRATION.md).

## Primary references

1. [Apple GarageBand for Mac User Guide](https://support.apple.com/guide/garageband/welcome/mac), especially its Tracks, Library, Editors, Mix/Automate and Smart Controls sections. The table paraphrases the documented feature headings, without claiming equivalent Ensemble behavior.
2. [Apple: convert audio with GarageBand / Export Song to Disk](https://support.apple.com/en-nz/120493) — documents MP3, quality and cycle/selected-region export controls.
3. [Apple: choose GarageBand input/output devices for an Aggregate Device](https://support.apple.com/en-ae/102171) — documents Audio/MIDI settings device choices. This does not establish any Ensemble device latency claim.
4. [Apple: compress a GarageBand project](https://support.apple.com/en-lamr/guide/garageband/gbndebf1310a/mac) — documents .band package contents and portability.

SoundFont evaluation: [FluidSynth synthesis-context API documentation](https://www.fluidsynth.org/api/synth-context.html) explains its render-thread context and API-specific thread-safety constraints. FluidSynth is **not integrated** into 0.3. A later backend needs actual load/note/render/free allocation/locking measurements and separate LGPL/asset licensing review; we do not infer callback safety solely from its real-time synthesizer description.
