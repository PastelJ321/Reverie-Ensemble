# Multisampler and sound packs — 0.2

The original Reverie synthesizer/scheduler/mixer is retained. A `SampleZone` maps key and velocity ranges to an immutable cached WAV, root pitch, source start/end, optional forward-loop points, one-shot flag and gain. Voices store an integer zone index and sample position; callback processing neither decodes files nor allocates/frees sample memory. Each track has 32 voices. Overlapping matching zones select the nearest root (first zone wins equal distances); layered simultaneous regions and round-robin are not implemented. Playback uses linear interpolation, not time stretching.

## SFZ subset

Supported sections: `<global>`, `<group>`, `<region>` with inherited values.

Supported opcodes:

```text
sample
key lokey hikey pitch_keycenter lovel hivel
 offset end loop_start loop_end
loop_mode=no_loop|one_shot|loop_sustain|loop_continuous
volume
ampeg_attack ampeg_decay ampeg_sustain ampeg_release
```

Keys accept MIDI numbers or note names with scientific middle C=C4=MIDI 60. Velocity ranges are MIDI 0–127. SFZ end/loop_end are inclusive and are converted to exclusive internal frame positions. `offset` is frames, while an audio clip's sourceOffset is seconds. `volume` is dB. Envelope times are seconds, sustain is percent. Envelope is currently preset-wide; differing per-region envelope values are rejected instead of silently applying the last one. `loop_sustain` exits the loop on note release; `loop_continuous` keeps looping through the release envelope; `one_shot` ignores ordinary note-off and ends at the sample end. The audition panic/release operation can still terminate previews.

Sample paths may contain unquoted spaces and use slash/backslash separators. Quoted paths, includes/macros, `<control>`, release triggers, round robin, CC/pedal/modulation, tuning/fine pitch, filters and arbitrary SFZ extensions are not supported. Unsupported opcodes cause a clear load error; invalid/missing WAVs and bad sample bounds also fail safely. SFZ files are limited to 2 MiB and 4,096 regions. There is no claim of general compatibility with full third-party SFZ instruments.

Project saves embed the supported mapping in version-2 JSON and collect each source WAV. Playback after reopening therefore does not depend on the original SFZ or on an installed pack directory. Preset exports retain separate WAV references; when distributing a standalone preset, also include its WAVs and update paths as needed. A weak shared sample cache deduplicates live references and enforces a 512 MiB decoded budget; history/active graphs keep samples alive through shared ownership.

## Pack manifest and installation

A pack is an **unpacked folder**, not a ZIP. Its `pack.json` includes id, version, name, author, source URL, license, exact payload installSize, an inventory of files with per-file size/SHA-256, instruments with id/category/tags/format/file, and an informational aggregate SHA-256 over the listed file hash strings. `assets/sounds/factory/pack.json` is a complete example and adds original source paths/URLs/root-mapping evidence. Runtime integrity uses every file's hash; the aggregate is for provenance, not a signature or authentication system.

Installer steps:

1. Read a manifest limited to 1 MiB and validate its identifier and metadata.
2. Reject absolute paths, `..` traversal and symlink escapes. Check actual file sizes, SHA-256 and total inventory size (maximum 1 GiB), and require every instrument mapping to be in the inventory.
3. Parse supported SFZ mappings and require their WAV references to be in the pack inventory.
4. Show source/license/size and ask the user in the app before copying. The app never silently downloads or installs a collection.
5. Copy to a fresh staging folder under the user library, verify the staging copy, then rename into place. Existing packs are not overwritten. On failure remove the staging folder, retain existing installed content and report the error.

Installed packs reside under Qt's writable AppLocalDataLocation `/SoundPacks/<pack-id>`. The bundled pack resides in app Resources/assets. Startup scans metadata/existence/size; installation and automated tests perform full hashing/decoding. A damaged or missing pack is not enumerated as playable. A WAV becoming unavailable after scanning is reported by the actual instrument loader.

External collection links open official source pages only on explicit user action; they are not download/install controls. Large source collections require selecting/adapting mappings into this documented subset. The UI does not claim an uninstalled external catalog is ready to play.

## SoundFont status

SF2 and SF3 are not SFZ. This release does not load them and does not link FluidSynth. No Bank/Program picker or placeholder SoundFont button is provided. A later optional backend must prepare/load/free off the callback, explicitly validate render/note API allocation/locking behavior, share the offline path, and document LGPL/build/transitive dependencies and user-provided SoundFont rights. Basic SFZ support and a real included acoustic pack are functional now; SoundFont hosting remains unfinished.
