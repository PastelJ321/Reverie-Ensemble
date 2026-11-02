# Reverie Ensemble 0.5

A free, open-source native DAW by QuadraJ. C++20, Qt 6 Widgets, Reverie Audio Core, miniaudio, sfizz, FluidSynth and CoreMIDI. Original application code is Apache-2.0; third-party code and sound assets retain their own licenses.

0.5 adds a full offline factory component, Meta piano/guitars, real CLAP instrument/effect hosting, ordered inserts, extended synthesis, channel dynamics/EQ and the Qt localization foundation. Existing projects, Musical Typing, MIDI/audio recording, SoundFonts, mixer and WAV export remain.

- **Factory:** 51 new recorded source instrument entries / 99 patches. The preserved 13-entry compact library and 14 synth patches plus one synthetic kit bring the selectable factory total to **128 patches**. Source entries can overlap; they are not all distinct physical instruments. Sound payload is about **1.48 GB**.
- **Plugins:** CLAP, validated with real Surge XT 1.3.4 and Surge XT Effects. Generic parameter editor, state/project restoration and offline WAV. AU/VST3 and native plugin GUIs are not implemented; see [capabilities](docs/PLUGIN_CAPABILITIES.md).
- **Production:** dual oscillators, mono/legato/portamento, filter envelope/LFO/FM; compressor, one parametric EQ band, peak safety limiter; ordered plugin inserts with bypass/wet/dry. Legacy DSP remains the default for old presets.
- **Localization:** English source, Korean TS/QM build and saved system/English/Korean preference. Korean wording is unfinished for Argon; restart applies language changes.

## Build the full app

On the tested macOS/arm64 environment (Qt 6.12.0):

```sh
brew install cmake qtbase qtsvg qttools libsndfile pkgconf
sh scripts/bootstrap_instruments.sh
# Explicit download of the selected factory payload, not the 9.33 GB full source collections:
python3 scripts/fetch_factory05.py --download
cmake -S . -B build-05-release -DCMAKE_BUILD_TYPE=Release -DREVERIE_REQUIRE_FACTORY=ON
cmake --build build-05-release -j 5
open build-05-release/ReverieEnsemble.app
```

The full build copies factory data into the `.app`, so normal first launch needs no sample download. A source-only build without that component keeps the old compact sounds and shows a missing-factory notice. New large samples stay in ignored build artifacts; they are not committed to this repository. `scripts/package_factory05.py` verifies/packages the factory component separately.

This local developer bundle still uses installed shared dependencies. Signed, notarized, clean-machine distribution and corresponding LGPL source/replacement packaging remain release work; see [distribution](docs/DISTRIBUTION.md).

## Play and record

Expand the Sound Browser with **⌘⇧B**, choose a factory instrument and Apply/Create Track. Open the keyboard with **⌘K**. Musical Typing: `A W S E D F T G Y H U J K`; `Z/X` octave, `C/V` velocity, `Tab` sustain. Arm a MIDI track, choose count-in, Record, play and Stop. The result is editable MIDI, then Save and WAV Export.

Audio Plugins opens the CLAP scanner/registry. Use standard CLAP folders or Add search folder, then Rescan. Plugin instruments also appear in the shared browser; Mixer → Inserts handles effects, and Parameters edits saved state. Plugins are separately installed software, not factory sounds. Source/license/installation state is shown; Meta standalone packs are identified when their recordings are already in Factory.

[Implementation report](docs/ENSEMBLE_0.5_REPORT.md) · [tests/screenshots](docs/TESTING.md) · [localization handoff](docs/LOCALIZATION_GUIDE.md) · [licenses](THIRD_PARTY_LICENSES.md) · [0.4 archive](docs/archive/README-0.4.md)
