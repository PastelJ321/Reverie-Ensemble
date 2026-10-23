# Paper & Graphite — Ensemble 0.2

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
