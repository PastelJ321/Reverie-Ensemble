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
