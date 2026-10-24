# Sound Library License Manifest — 0.2

Copyright 2026 QuadraJ and contributors applies to our software; source recordings retain CC0-1.0 and fonts retain SIL OFL 1.1.

## Shipped pack

- Pack ID: `quadraj.community-essentials`
- Version: `1.0.0`
- Display name: Community Essentials
- Author/credits: Versilian Studios: Sam Gossner; Ivy Audio: Simon Dalzell; sample editing: Elan Hickler / Soundemote; QuadraJ mappings
- Source: [VSCO 2 Community Edition](https://versilian-studios.com/vsco-community/)
- Original WAV revision: `440300901dfe9275fd84e0b7763af1f8443ae62e`
- License: CC0-1.0; original text: [CC0-1.0.txt](../assets/sounds/factory/CC0-1.0.txt)
- Payload: 58,673,138 bytes (55.96 MiB), 35 original WAVs, 9 instrument mappings
- Aggregate inventory checksum: `db8f9b15ea1a3d575c414f1a0e501411e749b0e50f272548a2bbf33a4fdb0d29`
- Exact per-file SHA-256, source URL and original path: [pack.json](../assets/sounds/factory/pack.json)

Each melodic instrument uses four distinct original pitch recordings; the percussion patch uses bass drum, snare and suspended cymbal hits. WAV files are unmodified upstream recordings. Our SFZ files are new compact mappings. One WAV is not duplicated under different instrument names. Root key facts were cross-checked against upstream SFZ revision `6dd651d55dde97fd4028699be9d4481f26917891`; several upstream naming conventions differ by an octave. The exact mapping source is recorded for each instrument. This is a sparse starter subset, not the complete VSCO 2 library or a full multi-mic/velocity-layer production library.

| Instrument | Category | Root mapping source |
|---|---|---|
| Upright Piano | Piano & Keys | [Upstream SFZ](https://github.com/sgossner/VSCO-2-CE/blob/6dd651d55dde97fd4028699be9d4481f26917891/VSUpright1.sfz) |
| Solo Violin | Strings | [Upstream SFZ](https://github.com/sgossner/VSCO-2-CE/blob/6dd651d55dde97fd4028699be9d4481f26917891/SViolinVib.sfz) |
| Violin Ensemble | Orchestral | [Upstream SFZ](https://github.com/sgossner/VSCO-2-CE/blob/6dd651d55dde97fd4028699be9d4481f26917891/ViolinEnsSusVib.sfz) |
| Cello Ensemble | Strings | [Upstream SFZ](https://github.com/sgossner/VSCO-2-CE/blob/6dd651d55dde97fd4028699be9d4481f26917891/CelloEnsSusVib.sfz) |
| Pizzicato Contrabass | Guitar & Bass | [Upstream SFZ](https://github.com/sgossner/VSCO-2-CE/blob/6dd651d55dde97fd4028699be9d4481f26917891/ContrabassPizz.sfz) |
| Concert Flute | Woodwinds | [Upstream SFZ](https://github.com/sgossner/VSCO-2-CE/blob/6dd651d55dde97fd4028699be9d4481f26917891/FluteExpVib.sfz) |
| Concert Trumpet | Brass | [Upstream SFZ](https://github.com/sgossner/VSCO-2-CE/blob/6dd651d55dde97fd4028699be9d4481f26917891/TrumpetSus.sfz) |
| Marimba | Melodic Percussion | [Upstream SFZ](https://github.com/sgossner/VSCO-2-CE/blob/6dd651d55dde97fd4028699be9d4481f26917891/Marimba.sfz) |
| Recorded Percussion Kit | Drums & Percussion | Original percussion hits mapped to GM 36/38/42 |

## Source research and exclusions

- [VSCO 2 Community Edition](https://versilian-studios.com/vsco-community/) identifies its original WAV library as CC0 and links the official source repository. Its formal [repository LICENSE](https://github.com/sgossner/VSCO-2-CE/blob/440300901dfe9275fd84e0b7763af1f8443ae62e/LICENSE) is retained verbatim. Upstream credits Sam Gossner and Simon Dalzell / Ivy Audio, with sample cutting by Elan Hickler / Soundemote. We preserve those credits and the source link. Older Readme courtesy requests do not change the formal CC0 dedication; this app is free and does not separately sell raw samples.
- [VCSL](https://versilian-studios.com/vcsl/) explicitly describes its collection as CC0, supplies raw WAV/SFZ resources and lists recorded instruments. Its larger collection was researched but is not included or auto-installed.
- [Karoryfer Free Samples](https://shop.karoryfer.com/pages/free-samples) states its free sample libraries are CC0 with an explicit **Marie Ork exception**; old downloads can contain earlier license text. No Karoryfer recording or voice bank is included in this release. The catalog is not treated as a blanket license for every possible asset.
- No commercial DAW samples, unknown-license sound packs, SF2/SF3 files or re-extracted recordings are included. External catalog links are clearly marked external; installed instruments are only the actual local mappings.

`fetch_factory.py` is an explicit developer download script, never invoked by app/CMake/CTest. It selects a pinned original revision; the checked-in WAVs let a normal clone build offline. Runtime user packs remain separately installed and user-supplied licenses must be reviewed before redistribution.

## Fonts

`assets/fonts/` includes IBM Plex Sans KR Regular/Medium/SemiBold and IBM Plex Mono Regular under SIL Open Font License 1.1. Original IBM copyright and reserved name are retained in OFL.txt / Mono-OFL.txt. Font binaries were reused byte-for-byte from existing related local project resources, then included independently for portable builds. No original sibling resource was changed or moved. The fonts are not sold separately, renamed, modified or relicensed as Apache-2.0. Qt system-font fallback remains available.

## Ensemble Essentials 1.0.0 (0.3 addition)

50 unchanged original WAVs, 4 instruments / 5 recorded patches, approximately 39 MiB. Combined with the 35 existing WAVs: **85 recordings, 13 instruments / 14 recorded patches**, approximately 95 MiB. The 9 synth/drum presets are separate synthesized sounds. No raw audio was normalized, truncated or converted during curation.

| Instrument / patch | Source / rights holder | Selected recordings |
|---|---|---|
| Electric Bass / Natural | Karoryfer Samples, D. Smolken, Fashionbass | 3 roots x 2 velocity layers x 2 round-robins = 12 |
| Hollowbody Electric Guitar / Natural | Karoryfer Samples, D. Smolken, Black And Green Guitars (black instrument) | 4 roots x 2 layers x 2 round-robins = 16 |
| Same guitar / Staccato | Same original instrument, distinct articulation | 4 recordings; counted as a patch, not another instrument |
| Concert Grand Piano / Natural | Versilian Studios / VCSL contributors, Kawai | 3 roots x 2 velocity layers = 6 |
| Acoustic Drum Kit / Natural | Versilian Studios / VCSL contributors | Kick/snare/closed & open hi-hat/high & low tom, 2 round-robins = 12 |

Primary rights evidence checked 2026-10-24:

- [Karoryfer official free-sample policy](https://shop.karoryfer.com/pages/free-samples) states CC0 for these free libraries; Marie Ork remains a separately licensed exception and is excluded.
- [Fashionbass individual LICENSE](https://github.com/sfzinstruments/karoryfer.fashionbass/blob/0703973b300a6bacb837602be9b588ab64732a72/LICENSE) and [Black And Green Guitars individual LICENSE](https://github.com/sfzinstruments/karoryfer.black-and-green-guitars/blob/b3b3249d37dc977a1a297bd2dc053e6d9b6b805c/LICENSE) were independently fetched and confirmed CC0; their original texts/readmes are included in the pack.
- [VCSL official page](https://versilian-studios.com/vcsl/) and [pinned VCSL README](https://github.com/sgossner/VCSL/blob/c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e/README.md) identify the sample library as CC0, with commercial/software redistribution allowed.
- Raw sample revision: VCSL c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e. Piano roots and source gain/offsets were cross-checked against [official Kawai SFZ](https://github.com/sgossner/VCSL/blob/dfcf4a4918771eee884b96ad4493de82ef84daf6/Chordophones/Zithers/Grand%20Piano,%20Kawai.sfz); root keys 48/72/96 correspond to its C2/C4/C6 filenames. Original fine-tuning is not implemented, so no tuning-perfect claim is made.
- Guitar roots follow [original Black guitar mapping](https://github.com/sfzinstruments/karoryfer.black-and-green-guitars/blob/b3b3249d37dc977a1a297bd2dc053e6d9b6b805c/Programs/modules/maps_black/ord.sfz); E3 is MIDI 40 in those source names. Bass Ab1/2/3 roots are 32/44/56 in its original mapping.

The complete file inventory, exact bytes, SHA-256, source paths/URLs, repository revisions, instrument grouping and calibration are in [the manifest](../assets/sounds/essentials03/pack.json). No entire multi-gigabyte collection was downloaded. [Developer reproduction script](../scripts/fetch_essentials03.py) requires an explicit --download flag; app/build/test never runs it. Versioned optional archive generation is documented in SOUND_FORMATS.md; no public downloadable release was published in this session.

## Image / code / audio separation

InstrumentArt.cpp creates original QPainter vector silhouettes and detailed illustrations under Apache-2.0. There are no downloaded instrument photos or Apple/third-party artwork assets. Audio is CC0 and fonts remain OFL. Sample permission is not asserted as permission for a photograph or logo. The original Demo Composition is Apache-2.0 content; its rendered guitar bounce uses the included CC0 source sound, not a microphone take or commercial DAW sample.
