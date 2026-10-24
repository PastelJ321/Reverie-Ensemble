# Paper & Graphite — Ensemble 0.3

Music sits on paper; tools live in graphite. The desktop remains Qt Widgets, with QPainter editing canvases. No web view, card dashboard, texture overlay, decorative gradient or fake hardware is used.

## Tokens and roles

`src/ui/DesignTokens.h` is the canonical palette. `Theme.cpp` resolves those tokens into the shared QSS and painter code uses the same roles.

| Dark shell role | Value | Light workspace role | Value |
|---|---|---|---|
| Background | #242321 | Editing paper | #E8E1D5 |
| Panel surface | #33312D | Editing ink | #302D28 |
| Elevated control | #4A453E | Secondary ink | #6A6258 |
| Panel border | #777065 | Grid | #CFC5B7 |
| Primary text | #EDE5D8 | Selection | #D4A67A |
| Secondary text | #BEB2A4 | Signal | #7EA89B |

Recording uses restrained red; actual clipping uses a brighter error red. Copper selection is not a warning. Small canvas labels use ink, not copper on paper. Track colors are muted, independent data colors, and selected clips have a dark outline; legacy projects retain saved colors.

## Typography and controls

IBM Plex Sans KR Regular/Medium/SemiBold and IBM Plex Mono Regular are reused from existing local QuadraJ/related project assets without changing the originals. The independent app bundles byte-identical font files with OFL notices in `assets/fonts`; it never requires a sibling project at runtime. If loading fails, Qt's system font is used safely.

The `ensemble.` wordmark uses 19 px semibold, panel/inspector headings 14 px, track/control text 12 px, secondary information 11 px, transport numbers 15 px Mono. Numeric spin controls use Plex Mono. There are no oversized marketing titles or decorative letter-spacing.

Controls are flat, normally 2 px radius, with one-pixel boundaries and distinct hover, pressed, checked, disabled and keyboard-focus states. Transport uses QPainter vector geometry cached at Retina resolution; Unicode/emoji play/record/brand glyphs are removed. Keyboard focus and button tooltips/accessible names identify icon-only Loop and Metronome controls. Scrollbars are nine pixels with a restrained handle.

## Layout and modules

- `Editor`: existing project/history/transport/file workflow controller; preserved public behavior.
- `Workspace`: transport, dock/splitter layout and control wiring, separated from the controller.
- `SoundBrowser`: search/category/favorites, provenance, preview/apply/drag, local pack installation, explicit external catalog links.
- `Timeline`: paper Arrangement, data clips, cached waveforms, fixed horizontal track headers, adjustable height, hover/selection, snap and loop span.
- `PianoRoll`: paper grid and 128 MIDI pitches; frozen visible keys/ruler, note gestures/duplication and audition.
- `AudioEditor`: paper waveform using cached bins; source-offset editing is in Selection Inspector.
- `InspectorPanel`: stable controls for the selected clip/note, hiding irrelevant fields; Track/Instrument tab has collapsible advanced parameters.
- `MixerView`: independent mode with aligned dB channel faders, real stereo meters, pan/mute/solo/master and actual EQ/Delay/Reverb controls.
- `Theme` / `DesignTokens`: shared visual rules, font loading and icon geometry.

Transport is 56 px. A separate compact edit row avoids overflow at 1280 px. Default browser width is about 250 px, Inspector about 280 px, and the lower editor is resizable/collapsible. Docks and scroll state persist through ordinary refreshes; swapping Mixer mode keeps the editing workspace alive. Track headers are painted in viewport coordinates while clips remain in project coordinates, so names/Mute/Solo remain fixed during horizontal scrolling.

## Review evidence

`docs/screenshots/0.2/` contains inspected Arrangement at 1440×900 and 1280×800, Piano Roll, Sound Browser, Mixer and Instrument Inspector. Additional local Retina/native snapshots are recorded in TESTING.md. QA corrected toolbar overflow, overly wide browser, unaligned master/channel faders, stale disabled Inspector rows and sampler oscillator controls that had no meaningful effect. These images are captures of real model-backed Qt widgets.

## 0.3 instrument and workflow refinement

The existing palette/fonts/flat geometry remain. InstrumentArt is original QPainter vector geometry: small track/list silhouettes and a detailed Paper illustration mode, including upright/grand, electric guitar/bass, strings/cello, flute/brass, mallets, drums and synths. No Apple or third-party imagery is imported. Illustrations are code assets under Apache-2.0; audio is separately CC0. They provide category identification, not photographic manufacturer-specific instrument depictions.

SoundBrowser separates instrument identities from patch rows, preserves category/search/favorites and adds recent selection. Artwork follows the selected family; patch metadata and real previews remain tied to actual preset loaders. Family controls expose tone, attack/release and room through supported DSP only. Transport position becomes an editable mono-font field; the ruler has an accessible drag hit area and distinct cycle handles. Long track names are elided instead of silently clipped.

Export Audio and Settings use the same shared QSS, compact rectangular controls, aligned forms and explicit status text. Export offers only working WAV combinations. Settings omits fake MIDI/autosave/online-update controls and explains their absence. Reviewed 0.3 application captures are in docs/screenshots/0.3; TESTING.md records exact sizes and limitations.
