# Factory instrument calibration — v1

Measured 2026-10-24 on Apple Silicon; 48 kHz, stereo float, fixed 1.5-second observation. Each row renders the actual synth/sampler through Reverie Audio Core, not the raw WAV alone. Low/middle/high sample-root keys (48/60/72 for synths; mapped keys for drums) are tested at velocity 0.3 / 0.7 / 1.0. Track and master are unity for measurement. Chords use root/major third/fifth. This is an engineering baseline, **not** a listening certification or LUFS measurement.

## Policy

- Raw WAV recordings are unchanged. SFZ source gain/offset corrections for the Kawai piano follow its pinned official mapping; the very quiet velocity-1 recordings need these source corrections before factory calibration.
- Family reference RMS: −25 dBFS for sustained strings/winds/synths, −30 dBFS for remaining categories. Use the median of the three medium-velocity roots as an anchor, then constrain strong single-note peak to −7 dBFS. Correction is bounded to −18…+30 dB. These are starting references, not per-note normalization, compression or automatic mastering. Short plucks and drums retain their high crest factors and may remain quieter by RMS.
- Audition compensation targets a conservative −16 dBFS strong-note reference (bounded −12…+8 dB) after calibration. The browser applies it only to its private preview copy. Project instrument calibration and mixer values are not changed by previewing.
- Metadata carries calibrationVersion, calibrationDb and auditionDb. The selected preset embeds linear gains and version in project format 3. Legacy v1/v2 load defaults to gain=1/version=0 and linear interpolation: existing mixes are not recalibrated. Selecting a new library patch explicitly adopts v1 calibration.
- The tool also reports each source WAV peak/RMS for reviewing range/velocity differences. Instrument targets and peak caps deliberately retain transient/sustain differences; a measured reduction in spread is not proof of perceptual equality.

Across the 23 measured patches/presets, the spread of representative medium-velocity median RMS decreased from **36.65 dB to 14.70 dB**. Remaining crest-factor differences and the guitar’s short high notes are audible-quality review priorities. No limiter or hidden compressor was added.

| Patch | Before RMS dBFS | Correction dB | After RMS dBFS |
|---|---:|---:|---:|
| Electric Bass | -32.36 | -2.38 | -34.74 |
| Hollowbody Electric Guitar | -36.85 | -1.51 | -38.36 |
| Concert Grand Piano | -29.60 | -0.40 | -30.00 |
| Acoustic Drum Kit | -50.73 | +18.29 | -32.44 |
| Hollowbody Electric Guitar / Staccato | -38.78 | -1.98 | -40.76 |
| Upright Piano | -34.99 | +4.99 | -30.00 |
| Solo Violin | -33.21 | +5.77 | -27.44 |
| Violin Ensemble | -53.39 | +24.32 | -29.07 |
| Cello Ensemble | -46.21 | +16.21 | -30.00 |
| Pizzicato Contrabass | -57.90 | +18.20 | -39.70 |
| Concert Flute | -33.76 | +1.26 | -32.50 |
| Concert Trumpet | -22.75 | -7.25 | -30.00 |
| Marimba | -48.94 | +15.22 | -33.72 |
| Recorded Percussion Kit | -44.11 | +7.68 | -36.43 |
| Simple Piano-like | -25.07 | +0.07 | -25.00 |
| Soft Pad | -23.09 | -1.91 | -25.00 |
| Synth Lead | -26.20 | +1.20 | -25.00 |
| Synth Bass | -22.70 | -2.30 | -25.00 |
| Pluck | -31.70 | +3.30 | -28.40 |
| Organ-like | -23.79 | -1.21 | -25.00 |
| Bell | -24.93 | -0.07 | -25.00 |
| Basic Synth | -21.25 | -3.75 | -25.00 |
| Studio Drums (synthetic) | -36.01 | -1.54 | -37.64 |

## Full data and reproduction

[Every key/velocity and chord result](measurements/0.3/instrument-levels.csv) · [Raw source levels](measurements/0.3/instrument-levels.csv.sources.csv) · [Demo metrics](measurements/0.3/demo-metrics.txt)

```sh
cmake --build build-03-release -j 4
build-03-release/reverie-calibrate assets/sounds out/levels.csv
 # Explicit developer operation to rewrite calibration metadata:
build-03-release/reverie-calibrate assets/sounds out/levels.csv --apply
```

The tool emits all measurements; see the CSV for pitch transitions rather than relying on one central note. Adjacent velocity layers are hard-switched, not crossfaded. The light guitar/piano selection does not cover every semitone or multiple release samples. Sustained and transient windows, spectral balance, stereo image, noise, loop transitions and apparent loudness need listening on calibrated monitors/headphones.
