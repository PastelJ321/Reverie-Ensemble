# Reverie Ensemble 0.5 User Guide

Reverie Ensemble is an offline macOS digital audio workstation for composing with recorded instruments, built-in synthesizers, CLAP plugins, and non-destructive WAV audio.

---

## Quick Start: Onboarding

When you open Reverie Ensemble, a starter project loads with four tracks (**Piano**, **Strings**, **Bass**, and **Percussion**) and a 4-bar arrangement.

- Press **Space** (or click **Play** in the transport bar) to hear the starter arrangement.
- Click a track header in the **Arrangement** pane to select that track and play its instrument.
- Press **Cmd+K** to open the **Virtual Keyboard**, enable **Musical Typing**, and play notes using your computer keyboard.
- Double-click any MIDI clip in the arrangement to edit its notes in the **Piano Roll** at the bottom of the window.

---

## 1. Creating Your First Project

1. Choose **File → New** (`Cmd+N`) to start a clean starter project, or **File → Project name…** to rename the current project.
2. Set the tempo (**BPM**, `20–300`), time signature (`4/4`, `3/4`, `6/8`, `7/8`), and **Loop beats** in the top transport bar.
3. Add tracks from the **Track** menu or by right-clicking an empty area of the Arrangement:
   - **Track → Add MIDI track** (`Cmd+Shift+N`): Creates a track for recorded sample instruments, built-in synthesizers, or CLAP instrument plugins.
   - **Track → Add audio track**: Creates a track for imported WAV files or microphone recording.
4. Double-click an empty lane on a MIDI track to create a 4-beat MIDI clip.

---

## 2. Choosing Instruments

The **Sound Library** dock on the left lists all installed instruments and patches.

1. Select a MIDI track in the Arrangement.
2. Filter by category (**Piano & Keys**, **Strings**, **Woodwinds**, **Brass**, **Guitar & Bass**, **Drums & Percussion**, **Synthesizers**, **World Instruments**, **Orchestral**) or type in **Search instruments / tags**.
3. Click an instrument to view its available **Patches** (such as *Concert Sustain*, *Soft Felt*, *Pizzicato*, or *Fingered*).
4. Click **Listen** to audition a short phrase, or click **Apply** (or double-click an instrument) to assign it to the selected MIDI track.
5. Click **New instrument track** to create a dedicated track with the selected instrument and patch already loaded.
6. Click **Expand ↔** (`Cmd+Shift+B` or **View → Expand Instrument Browser**) to open the three-column browser showing categories, instruments, and patch/license details side by side.

---

## 3. Installing Sound Libraries and Importing User Instruments

Reverie Ensemble bundles the **Factory Library** and **Community Essentials** (`CC0-1.0`) for offline use. Optional packs are managed without background downloads.

### Installing Optional or Local Sound Packs

1. In the Sound Library dock, click **Available collections…** (or open the **⋯** tools menu and choose **Sound Library Manager…**).
2. Select a pack under **AVAILABLE PACKS** to inspect its download size, patch count, `sfizz` backend requirement, and license terms.
3. Click **Download / Install / Update** and confirm the license prompt. Ensemble verifies every file against its `SHA-256` inventory before installing.
4. For offline installation, click **Install local folder…** and select a verified unpacked directory containing `pack.json`.
5. To remove an optional pack, select it under **INSTALLED** and click **Remove optional pack…**. Bundled factory sounds cannot be deleted.

### Importing SFZ and SoundFont Files

1. Click the **⋯** button in the Sound Library dock and choose **Import SFZ / SoundFont…**.
2. Select a `.sfz`, `.sf2`, or `.sf3` file from disk.
3. For multi-preset SoundFont files, choose the desired **Bank : Program** when prompted. Imported instruments appear under the **User Instruments** category.

---

## 4. Playing with the Computer Keyboard (Musical Typing)

You can play and record any MIDI track using either an external CoreMIDI keyboard or your Mac keyboard.

1. Select a MIDI track in the Arrangement.
2. Choose **View → Virtual Keyboard / Musical Typing** (`Cmd+K`) or click **Keyboard** in the transport bar.
3. Check **Musical Typing** in the Virtual Keyboard dock:
   - **White keys**: `A` `S` `D` `F` `G` `H` `J` `K` (C to C)
   - **Black keys**: `W` `E` `T` `Y` `U` (C♯, D♯, F♯, G♯, A♯)
   - **Octave down / up**: `Z` / `X`
   - **Velocity down / up**: `C` / `V`
   - **Sustain pedal**: Hold `Tab`
4. You can also click and drag across the on-screen keys (`MIDI 21–108`) with the mouse.

---

## 5. Recording MIDI

1. Select the MIDI track you want to record onto.
2. Click **Arm** in the track strip (or choose **Transport → Arm selected track**, or click the `●` button on the track header). Only one track can be armed at a time.
3. Select a pre-roll option in the transport bar (**Count-in: None**, **Count-in: 1 bar**, or **Count-in: 2 bars**) and enable **Metronome** if desired.
4. Press **Record** (`R`) in the transport bar (or **Transport → Record**).
5. Play notes on your MIDI controller, the Virtual Keyboard, or via Musical Typing.
6. Press **Stop** (or **Finish MIDI**) to end the take. Held notes are closed automatically at the stop beat, and the recorded notes are placed in a new or merged MIDI clip on the track.

---

## 6. Editing Notes in the Piano Roll

Double-click any MIDI clip in the Arrangement (or click **Piano editor** in the transport bar) to open the **Piano Roll**.

- **Add a note**: Click an empty grid cell.
- **Preview pitch**: Click the piano keys along the left edge.
- **Move a note**: Drag the body of a note horizontally (beat position) or vertically (pitch).
- **Resize a note**: Drag the right edge of a note.
- **Select multiple notes**: Hold `Shift` and drag a selection rectangle.
- **Delete notes**: Select one or more notes and press `Delete` or `Backspace`.
- **Quantize**: Select notes (or leave none selected to affect all notes in the clip) and click **Quantize** (`Cmd+U`), using the active **Snap** grid (`1/4`, `1/8`, `1/16`, `1/32`).
- **Context menu commands**: Right-click a note to open **Delete**, **Duplicate**, **Quantize**, **Set Velocity…** (`1–127`), **Set Length…**, **Transpose…** (`−48` to `+48` semitones), or **Note Properties…**.

---

## 7. Recording and Editing Audio

### Recording from a Microphone or Audio Interface

1. Create or select an **Audio Track** (**Track → Add audio track**).
2. Arm the audio track (**Arm**) and ensure any other armed track is disarmed.
3. Press **Record** (`R`). If macOS prompts for microphone access, grant permission in **System Settings → Privacy & Security → Microphone**.
4. Press **Stop** (or **Finish take**) when done. Ensemble writes a WAV file into the project media folder and places a clip on the track.

> **Note:** Recording is linear on one armed track at a time. Live audio input monitoring and hardware latency compensation are not provided in 0.5.

### Importing and Trimming WAV Clips

1. Choose **File → Import WAV…** (or right-click an audio track lane and choose **Import Audio…**).
2. Drag the clip horizontally to position it on the timeline.
3. Drag the right edge of the clip to trim its playback length non-destructively. You can also adjust **Source offset / s** and **Length / beats** in the right-hand **Selection** inspector.

---

## 8. Using the Mixer and Channel Processing

Click **Mixer** (`Cmd+M`) in the transport bar to switch the center view from Arrangement to the channel strip mixer.

- **Track Strips**: Each track provides a level fader (**dB**), **Pan** (`L`/`R`), **Mute** (`M`), **Solo** (`S`), **Low** and **High** shelf EQ, and **Delay** / **Reverb** send controls.
- **Channel Processing**: Click **Processing…** on any track or the **Master** strip to open the dedicated DSP chain:
  - **Input gain (dB)** and **Output gain (dB)**
  - **Parametric EQ**: Enable, **Frequency (Hz)**, **Gain (dB)**, and **Q**
  - **Compressor**: Enable, **Threshold (dB)**, **Ratio**, **Attack (ms)**, **Release (ms)**, and **Makeup (dB)**
  - **Peak limiter**: Enable, **Threshold (dB)**, and **Release (ms)**
- **Instrument Controls**: For MIDI tracks using recorded instruments, the track strip exposes **Tone / low-pass**, **Release**, **Room / reverb**, and **Bow attack** (for bowed strings). For built-in synthesizers, click **Synth controls…** to edit oscillator blend, filter envelope, FM index, LFO, vibrato, detune, portamento, and monophonic/legato mode.

---

## 9. Adding CLAP Plugins

Reverie Ensemble hosts stereo **CLAP** (`1.x`) instrument and effect plugins. Audio Units (AU) and VST3 plugins are not supported in this build.

### Scanning and Loading Plugins

1. Choose **File → Audio Plugins…** (or click **Audio Plugins…** in the Sound Library dock).
2. Ensemble scans standard CLAP directories (`/Library/Audio/Plug-Ins/CLAP`, `~/Library/Audio/Plug-Ins/CLAP`). Click **Add search folder…** to add a custom directory, or **Rescan** to refresh.
3. To load an **Instrument plugin**:
   - Select it in the plugin table and click **Create track** to create a new MIDI track driven by that CLAP instrument, or select an existing MIDI track and click **Apply**.
4. To load an **Effect plugin**:
   - Select the target track (or click **Inserts…** on a Mixer channel strip) and click **Insert**.

### Managing Track Inserts and Parameters

1. Click **Inserts…** on any Mixer strip (or choose **File → Plugin Parameters…** for the selected track).
2. In **Track inserts**, you can **Add…** an effect, open its searchable **Parameters…** table, toggle **Bypass**, adjust **Wet** mix (`0–100%`), **Move up**, or **Remove** the slot.

---

## 10. Saving Projects and Media Collection

1. Choose **File → Save** (`Cmd+S`) or **File → Save As…** (`Cmd+Shift+S`).
2. Ensemble saves the project file (`.reverie`) alongside a sibling `<ProjectName>.media/` directory containing imported and recorded WAV files and copied user samples.
3. External SFZ libraries that reference outside sample trees keep their external paths; keep those libraries available on disk when moving projects between folders.
4. Reverie Ensemble uses explicit manual saves and prompts before closing an unsaved project. It does not perform background auto-saves or online checks.

---

## 11. Exporting WAV Audio

1. Choose **File → Export Audio…** (`Cmd+E`).
2. Click **Choose…** to pick the output `.wav` file path.
3. Configure the export settings:
   - **Sample rate**: `44100 Hz`, `48000 Hz`, or `96000 Hz`
   - **Bit depth**: `16-bit PCM`, `24-bit PCM`, or `32-bit float`
   - **Channels**: **Stereo** or **Mono (L+R / 2)**
   - **Range**: **Entire project**, **Loop / cycle range**, or **Selected clip range**
   - **Include instrument release and effect tail**: Appends reverb/delay decay and note release after the end beat.
   - **Normalize peak to −1 dBFS**: Applies a single linear gain adjustment so the highest peak reaches `−1.0 dBFS` without dynamic compression.
   - **TPDF dither for integer PCM**: Applies triangular dither when exporting `16-bit` or `24-bit` PCM.
4. Click **Export**. Ensemble renders to a temporary staging file first; cancelling during render leaves any existing destination file untouched.

---

## 12. Common Errors and Troubleshooting

| Message / Symptom | Cause | Solution |
|---|---|---|
| **No output device. Use Reconnect audio; offline WAV export is available.** | No CoreAudio playback device is currently active or available. | Connect headphones or an audio interface, then click the audio status badge in the transport bar (**Reconnect audio / clear clip**) or open **File → Settings… → Audio** and click **Refresh devices** → **Apply audio configuration**. |
| **Microphone access denied** | macOS blocked audio input capture for Reverie Ensemble. | Open **System Settings → Privacy & Security → Microphone**, enable **Reverie Ensemble**, and try recording again. |
| **Arm one recording track at a time.** | Multiple tracks have **Arm** enabled simultaneously. | Disarm other tracks so only the target MIDI or Audio track is armed. |
| **Missing WAV — relink by importing source** | A WAV file referenced by an audio clip was moved or deleted. | Re-import the WAV file or restore it into the project's `.media/` folder. Existing clip positions are preserved and play silently until relinked. |
| **CLAP init failed / Could not scan** | The plugin bundle is damaged, unsupported, or missing stereo I/O. | Verify the `.clap` bundle resides in a configured search folder, confirm it provides stereo audio ports, and click **Rescan** in **Audio Plugins…**. |
| **Pack is in use** | You attempted to remove an optional sound pack while the current project uses one of its instruments. | Change the instrument on any track using that pack (or remove the track) before uninstalling the pack in **Sound Library Manager**. |
| **CLIPPING — lower track/master levels** | Master output exceeded `0 dBFS`. | Lower individual track faders or the **Master** fader, enable the **Peak limiter** in **Channel processing**, and click the audio status badge to clear the clipping indicator. |

---

## 13. Sound Library Instrument Guide

The bundled **Ensemble 0.5 Factory Library** (`assets/sounds/factory05`) and **Community Essentials** (`assets/sounds/essentials03`) combine public-domain (`CC0-1.0`) recordings from **Versilian Community Sample Library (VCSL)**, **VSCO 2 Community Edition**, **Splendid Grand**, **Salamander Drumkit**, and **Karoryfer Samples** with calibrated built-in synthesis patches.

### Piano & Keys
- **Concert Grand Piano** (`factory.concert-grand`): Wide-range acoustic grand piano recorded from the Akai Steinway III (`VCSL / Splendid Grand`, `CC0-1.0`). Includes *Concert Sustain*, *Intimate Felt*, and *Bright Studio* patches. Suitable for solo piano, chamber accompaniment, and pop/cinematic arrangements. Supports tone/low-pass, release, and room/reverb controls.
- **Upright Piano** (`factory.upright-piano`): Knight K10 upright piano (`VCSL`, `CC0-1.0`) with a direct, woody midrange. Includes *Room Upright*, *Soft Practice*, and *Tack Piano* patches for intimate songwriting, lo-fi, and small ensemble textures.
- **Electric Piano** (`factory.electric-piano`): Velocity-layered electro-mechanical tine piano (`VCSL`, `CC0-1.0`) with *Stage Mark*, *Warm Lounge*, and *Bell Tines* patches.
- **Pipe / Reed Organ** (`factory.organ`): Positive organ (`8'` and `4'` registrations, `VCSL`, `CC0-1.0`) with steady sustain for liturgical, baroque, and ambient pad layers.

### Strings & Harp
- **Solo Violin** (`factory.violin`) & **Solo Viola** (`factory.viola`): Bowed solo strings (`VCSL`, `CC0-1.0`) offering *Arco Vibrato* / *Lyric Sustain*, *Warm Chamber*, and *Pizzicato* patches. Expose **Bow attack**, **Release**, and **Room / reverb** shaping.
- **Solo Cello** (`factory.cello`): Expressive solo cello (`VCSL`, `CC0-1.0`) covering bass-to-tenor register with *Lyric Sustain*, *Warm Legato*, and *Pizzicato* patches.
- **Upright Bass** (`factory.contrabass`): Acoustic double bass (`VCSL`, `CC0-1.0`) with *Orchestral Arco* and *Jazz Pizzicato* patches for orchestral foundations and acoustic rhythm sections.
- **String Section** (`factory.string-ensemble`): Multi-zone ensemble string mapping (`VCSL`, `CC0-1.0`) with *Ensemble Sustain*, *Soft Chamber*, and *Marcato Bow* patches.
- **Concert Harp** (`factory.concert-harp`): Diatonic concert harp plucks (`VCSL`, `CC0-1.0`) with natural resonance, offering *Concert Plucked* and *Warm Rolled* patches.

### Woodwinds & Brass
- **Concert Flute** (`factory.flute`), **Oboe** (`factory.oboe`), **Clarinet** (`factory.clarinet`), **Bassoon** (`factory.bassoon`): Orchestral woodwinds (`VCSL`, `CC0-1.0`) with sustained and agile articulation variants (*Concert Sustain*, *Soft Breath*, *Staccato*, *Chamber Reeds*).
- **French Horn** (`factory.french-horn`), **Trumpet** (`factory.trumpet`), **Trombone** (`factory.trombone`), **Tuba** (`factory.tuba`): Orchestral brass (`VCSL`, `CC0-1.0`) providing open sustain, warm muted/chorale filtering, and accented brass patches.

### Guitar & Bass
- **Acoustic Guitar** (`factory.acoustic-guitar`): Plucked acoustic guitar (`VCSL`, `CC0-1.0`) with *Natural Picked*, *Warm Fingered*, and *Muted Pluck* (`essential.guitar-staccato`) patches.
- **Electric Guitar** (`factory.electric-guitar`): Direct single-coil electric guitar (`VCSL`, `CC0-1.0`) with *Clean Direct*, *Warm Jazz*, and *Muted Pluck* patches.
- **Electric Bass** (`factory.electric-bass`): Fingered electric bass (`Karoryfer / Fashionbass`, `CC0-1.0`) with two velocity layers and round-robin variations (*Fingered Studio*, *Deep Sub*, *Pick Attack*).

###Mallet, World & Percussion
- **Marimba** (`factory.marimba`), **Xylophone** (`factory.xylophone`), **Vibraphone** (`factory.vibraphone`), **Glockenspiel** (`factory.glockenspiel`), **Tubular Bells** (`factory.tubular-bells`): Tuned orchestral percussion (`VCSL`, `CC0-1.0`).
- **Kalimba** (`factory.kalimba`) & **Balafon** (`factory.balafon`): Plucked thumb piano and West African gourd-resonated xylophone (`VCSL`, `CC0-1.0`).
- **Studio Drum Kit** (`factory.drum-kit`): Multi-sampled acoustic drum kit (`VCSL / Salamander Drumkit`, `CC0-1.0`) mapped across General MIDI drum keys (`MIDI 35–59`: kick, snare, rimshot, hi-hats, toms, crash, and ride), with *Studio Kit*, *Tight Dry Kit*, and *Room Kit* patches.
- **Orchestral Percussion** (`factory.orchestral-percussion`): Concert hall percussion kit (`VCSL`, `CC0-1.0`) featuring *Timpani & Hall*, *Gran Cassa*, and *Cymbal Swell* patches.

### Built-In Synthesizers
- **Warm Analog Pad** (`factory.warm-pad`) & **Glass Choir Pad** (`factory.glass-pad`): Polyphonic subtractive/FM pad synthesizers with full envelope, filter, LFO, and stereo chorus/reverb controls.
- **Analog Bass Synth** (`factory.analog-bass`) & **Mono Synth Lead** (`factory.mono-lead`): Low-end bass and expressive lead synthesizers supporting monophonic legato and portamento glide.
- **FM Electric Keys** (`factory.fm-keys`) & **Plucked Synth Harp** (`factory.pluck-arp`): Percussive FM tine/bell keys and short-decay arpeggio plucks.
