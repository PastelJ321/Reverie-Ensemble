# 0.2 priorities and SF2 decision

0.1 provides a working composition/playback/mix/save/export path, WAV import, a pitched sampler and a streaming recording implementation. The highest priority for 0.2 is to validate and harden those paths under real recording sessions.

1. Test real microphones, permission denial, device hotplug, disk-full recovery, long sessions and round-trip latency on Apple Silicon and Intel. Add input monitoring, device/rate/buffer selection and recording latency compensation. Detect backend errors without misleading output status.
2. Add smoothing to transport/master/clip boundaries, better sampler resampling, shared media cache management across imports, automated stress/profiling and explicit overload reporting. Generalize tempo maps and verify varying block sizes/sample rates.
3. Add piano keyboard audition, all MIDI octaves, external MIDI input and standard MIDI file import/export; loop range, metronome, split/left-trim/fades, grid alternatives and a more complete mixer. Preserve Undo transaction semantics.
4. Integrate an optional SF2 instrument backend after licensed test fixtures and real-time behavior have been validated; retain Reverie graph/scheduler/mixer/recorder/rendering. Expand original presets and user-managed, licensed instrument metadata.
5. Produce self-contained, signed/notarized macOS releases with exact corresponding Qt/transitive dependency source archives, license notices, user relinking instructions and Intel/universal testing. Then CI and platform-specific save/path/device tests for Windows/Linux.

## SF2 / FluidSynth assessment

FluidSynth is a possible **instrument backend**, not a replacement engine. Its official project uses LGPL-2.1-or-later; optional build features can change the binary's licensing. See the [upstream repository](https://github.com/FluidSynth/fluidsynth), [licensing FAQ](https://www.fluidsynth.org/wiki/LicensingFAQ/), and [API documentation](https://www.fluidsynth.org/api/).

The proposed module would dynamically link an LGPL-compatible FluidSynth build, load SoundFonts and enumerate banks/programs off the audio thread, preconfigure polyphony and sample rate, render into Reverie buffers, and receive sample-aligned MIDI events from the same scheduler. Presets would hold a path, bank/program and sound-pack provenance/license. Any FluidSynth allocation, voice growth, file access or locks during rendering must be measured and addressed before enabling it in the real-time callback. It must share the same offline renderer path.

0.1 does **not** link FluidSynth, load SF2, ship a SoundFont, or expose a nonfunctional SF2 button. A tested redistributable SoundFont and a validated callback adapter are not part of this initial build. No third-party sound pack was automatically downloaded. The `Instrument` boundary and separate sample/provenance fields provide the extension point. LGPL notices, exact corresponding backend source and relinkability would be added when the backend is actually included.
