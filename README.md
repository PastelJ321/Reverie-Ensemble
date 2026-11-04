# Reverie Ensemble 0.5.1

A free, open-source native DAW by QuadraJ. C++20, Qt 6 Widgets, Reverie Audio Core, miniaudio, sfizz, FluidSynth and CoreMIDI. Original application code is Apache-2.0; third-party code and sound assets retain their own licenses.

0.5.1 stabilizes the 0.5 milestone with complete Korean UI integration, localization bug fixes, reusable build profiles, and standalone bundle staging. It preserves all 0.5 offline factory instruments, Meta piano/guitars, real CLAP instrument/effect hosting, ordered inserts, extended synthesis, channel dynamics/EQ, Musical Typing, MIDI/audio recording, SoundFonts, mixer, WAV export, and v1–v5 project compatibility.

- **Factory:** 51 new recorded source instrument entries / 99 patches. The preserved 13-entry compact library and 14 synth patches plus one synthetic kit bring the selectable factory total to **128 patches**. Source entries can overlap; they are not all distinct physical instruments. Sound payload is about **1.48 GB**.
- **Plugins:** CLAP, validated with real Surge XT 1.3.4 and Surge XT Effects. Generic parameter editor, state/project restoration and offline WAV. AU/VST3 and native plugin GUIs are not implemented; see [capabilities](docs/PLUGIN_CAPABILITIES.md).
- **Production:** dual oscillators, mono/legato/portamento, filter envelope/LFO/FM; compressor, one parametric EQ band, peak safety limiter; ordered plugin inserts with bypass/wet/dry. Legacy DSP remains the default for old presets.
- **Localization:** English source, complete Korean TS/QM build (`translations/ensemble_ko_KR.ts`, 636/636 entries translated including runtime engine error translation), and saved system/English/Korean preference (`Settings → General → Language`). Restart applies language changes without modifying project state; save your project before restarting.

## Build the full app

On the tested macOS/arm64 environment (Qt 6.12.0):

```sh
brew install cmake qtbase qtsvg qttools libsndfile pkgconf
sh scripts/bootstrap_instruments.sh
# Explicit download of the selected factory payload, not the 9.33 GB full source collections:
python3 scripts/fetch_factory05.py --download

# Standard reusable build profiles (build-dev, build-release, build-core, build-sanitize):
./scripts/build.sh release

# Stage a self-contained app bundle in dist/current/ReverieEnsemble.app:
./scripts/build.sh dist
open dist/current/ReverieEnsemble.app
```

Development builds (`build-dev/`, `build-release/`) symlink `out/factory05` into `Contents/Resources/FactoryLibrary` by default (`-DREVERIE_BUNDLE_FACTORY_COPY=OFF`) so the 1.48 GB sample payload is not duplicated across every build directory. Staging `./scripts/build.sh dist` copies `FactoryLibrary`, `translations`, and bundled `libsfizz`/`libfluidsynth` dynamic libraries into `dist/current/ReverieEnsemble.app` so the staged bundle runs without depending on build-tree paths. Use `python3 scripts/clean_builds.py` (Dry Run by default) to inspect or reclaim legacy versioned build directories (`--apply`).

Signed, notarized, clean-machine distribution and corresponding LGPL source/replacement packaging remain release work; see [distribution](docs/DISTRIBUTION.md).

## Play and record

Expand the Sound Browser with **⌘⇧B**, choose a factory instrument and Apply/Create Track. Open the keyboard with **⌘K**. Musical Typing: `A W S E D F T G Y H U J K`; `Z/X` octave, `C/V` velocity, `Tab` sustain. Arm a MIDI track, choose count-in, Record, play and Stop. The result is editable MIDI, then Save and WAV Export.

Audio Plugins opens the CLAP scanner/registry. Use standard CLAP folders or Add search folder, then Rescan. Plugin instruments also appear in the shared browser; Mixer → Inserts handles effects, and Parameters edits saved state. Plugins are separately installed software, not factory sounds. Source/license/installation state is shown; Meta standalone packs are identified when their recordings are already in Factory.

[User guide (EN)](docs/USER_GUIDE.md) · [사용자 가이드 (KO)](docs/USER_GUIDE.ko.md) · [terminology](docs/TERMINOLOGY.md) · [localization review](docs/TRANSLATION_REVIEW.md) · [localization guide](docs/LOCALIZATION_GUIDE.md) · [implementation report](docs/ENSEMBLE_0.5_REPORT.md) · [tests/screenshots](docs/TESTING.md) · [licenses](THIRD_PARTY_LICENSES.md) · [0.4 archive](docs/archive/README-0.4.md)
