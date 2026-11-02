# Plugin capability matrix — 0.5

| Capability | CLAP | Audio Units | VST3 |
|---|---|---|---|
| Standard/user folder discovery | Yes, isolated executable, per-candidate timeout | No host | No host |
| Instrument MIDI → stereo audio | Yes, MIDI 1 dialect required | No | No |
| Effect inserts | Yes, ordered, bypass and wet/dry | No | No |
| Parameters | Generic numeric/stepped/read-only editor | No | No |
| State/project/offline WAV | Yes, opaque state + ID/path/vendor/version | No | No |
| Native plugin GUI | No | No | No |
| Latency/tail | Reported; bounded tail used | No | No |
| Delay compensation / sidechain routing | No | No | No |
| Runtime crash isolation | No; scanner is isolated | No | No |

CLAP headers are vendored at `a47f6badb49d948fd009998f28309cdab78979c9` under MIT. Lifecycle/state/parameter operations are dispatched to the application main thread. Processing uses fixed buffers up to 256 frames and a 4,096-event queue. Modules share a bounded cached handle until shutdown; loaded binary updates require restarting Ensemble. A state extension is required for the UI save/restore workflow. Thread roles are exposed through `clap.thread-check` and independently asserted by the test effect. Stereo main I/O is supported, up to eight stereo ports; auxiliary outputs are discarded and unconnected auxiliary inputs receive silence. Mono, arbitrary bus layouts, note-expression/MPE, MIDI output, host automation recording and native plugin GUIs are outside this host's capability set.

The host passes tempo, beat/second position, time signature and transport state. Plugins may report latency; **tracks are not automatically delay-compensated**. Plugin restart requests currently silence the instance until it is prepared again. Main-thread callbacks are serviced during preparation/parameter editing; general timer/fd/thread-pool/GUI extensions are not advertised. A plugin requiring those extensions can be incompatible even if another CLAP plugin works.

Search roots: `~/Library/Audio/Plug-Ins/CLAP`, `/Library/Audio/Plug-Ins/CLAP` and saved user folders. Rescan runs each bundle in `reverie-plugin-scan`, with a 15-second limit. A deliberately aborting fixture verifies that the parent UI remains alive. Runtime processing occurs in Ensemble's process and cannot contain an arbitrary plugin crash. Only load trusted installed binaries. Loading/state errors are reported; missing instruments remain silent/recoverable and missing effects are bypassed. State payloads are limited to 8 MiB; at most 16 inserts per track.

Surge XT **1.3.4**, official macOS plugins-only package: actual instrument discovery, parameter/state editing, Musical Typing/MIDI capture, save/reopen and WAV; actual Surge XT Effects loading, parameter/state restoration and offline processing. Independent original gain/bias and crash fixtures test ordered inserts, reorder, bypass, wet/dry and thread roles. These fixtures are QA material, not factory instruments. Dexed and Vital are not verified. No Surge/Dexed/Vital binaries, GPL synth source or Vital presets are shipped with Ensemble.

SDK references: [CLAP](https://github.com/free-audio/clap), [Apple Audio Units](https://developer.apple.com/documentation/audiotoolbox/audio_unit_v2_c_api), [VST3 MIT SDK](https://github.com/steinbergmedia/vst3sdk). AU/VST3 are future work, not advertised working formats.
