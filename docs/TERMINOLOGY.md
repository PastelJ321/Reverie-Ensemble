# Reverie Ensemble Terminology Guide (`en` / `ko_KR`)

This document defines the canonical English and Korean terminology for **Reverie Ensemble 0.5**. Use this reference when writing UI strings, updating `translations/ensemble_ko_KR.ts`, authoring user documentation, or describing instruments and plugins.

---

## 1. Core Principles

1. **Clarity and Brevity**: Use concise, standard terminology familiar to Korean macOS music production users.
2. **One Concept, One Term**: Do not use different labels for the same command across menus, toolbars, and context menus. Conversely, do not collapse distinct technical concepts into a single word.
3. **No Redundant Parenthetical English**: Do not append English in parentheses after established Korean terms (for example, write `벨로시티`, not `벨로시티(Velocity)`; write `톱니파`, not `톱니파(Saw)`).
4. **Appropriate Register**:
   - **Menus, Buttons, Labels, Headers**: Noun phrases or compact action phrases (`새 프로젝트`, `오디오 내보내기…`, `트랙 삭제`, `다시 검색`).
   - **Status Bar, Tooltips, Descriptions, Dialog Prompts**: Clear, polite declarative or imperative sentences (`-습니다`, `-세요`). Avoid overly casual (`-해요`) or promotional phrasing.
5. **Actionable Error Messages**: State what failed, what the user can do next, and whether current work or files remain safe.

---

## 2. Core DAW & Workspace Terms

| English | 한국어 | Usage & Context |
|---|---|---|
| Arrangement | 편곡 | Timeline / track arrangement view (`ARRANGEMENT` header, `Arrange` view button) |
| Track | 트랙 | General track unit |
| Audio Track | 오디오 트랙 | WAV playback and microphone recording track |
| Instrument Track | 악기 트랙 | Track hosting a built-in synth, recorded sample instrument, or CLAP instrument |
| MIDI Track | MIDI 트랙 | Track containing MIDI clips routed to an instrument |
| Clip | 클립 | Arrangement container for MIDI notes or a non-destructive WAV slice |
| Region | 리전 / 구간 | Use `리전` for SFZ sample mapping definitions (`SFZ 리전`); use `구간` for timeline time ranges (`반복 구간`, `출력 구간`) |
| Piano Roll | 피아노 롤 | Note editor for MIDI clips (`PIANO ROLL`, `Piano editor`) |
| Mixer | 믹서 | Channel strip and master bus view |
| Inspector | 인스펙터 | Right-hand selection property panel (`Selection` / `선택 항목`) |
| Sound Library | 사운드 라이브러리 | Instrument browser and pack catalog |
| Sound Library Manager | 사운드 라이브러리 관리자 | Dialog for downloading, verifying, installing, and removing optional packs |
| Virtual Keyboard | 가상 건반 | On-screen MIDI keyboard (`Cmd+K`) |
| Musical Typing | 키보드 연주 | Playing notes with the computer keyboard (`A W S E D F T G Y H U J K`) |

---

## 3. Transport, Recording & Editing Terms

| English | 한국어 | Usage & Context |
|---|---|---|
| Transport | 트랜스포트 | Top playback/recording bar and `&Transport` menu |
| Play / Pause | 재생 / 일시정지 | Transport playback toggle |
| Stop | 정지 | Stops transport or finishes an active recording take |
| Record | 녹음 | Starts linear MIDI or audio capture |
| Record Arm / Arm | 녹음 준비 | Prepares a track for recording (`Arm`, `Arm Recording`, `Arm selected track`) |
| Input Monitoring | 입력 모니터링 | Real-time pass-through of live audio input (not provided in 0.5; state clearly in docs/help) |
| Metronome | 메트로놈 | Audible click during playback and recording |
| Click level | 메트로놈 볼륨 | Metronome output level slider |
| Count-in | 녹음 전 카운트 | Pre-roll bars before recording begins (`없음`, `1마디`, `2마디`) |
| Loop / Loop playback | 반복 / 반복 재생 | Cycle playback over the selected beat range |
| Loop / cycle range | 반복 구간 | Export or playback range between loop markers |
| Follow | 자동 스크롤 | Scrolls timeline automatically when playhead leaves visible range |
| Snap | 스냅 | Grid alignment in arrangement and piano roll |
| Quantize | 퀀타이즈 | Aligns selected MIDI note starts to the active grid |
| Velocity | 벨로시티 | MIDI note strike intensity (`1–127`) |
| Transpose | 조옮김 | Shifts selected MIDI notes by semitones |
| Semitones | 반음 단위 | Pitch shift unit |
| Split at Playhead | 재생 헤드 위치에서 분할 | Splits a clip at the current playhead beat |
| Duplicate | 복제 | Duplicates a clip, note selection, or track |
| Automation | 오토메이션 | Time-varying parameter curves (note: MIDI tempo automation is unsupported on import) |

---

## 4. Mixer, Channel Processing & Plugin Terms

| English | 한국어 | Usage & Context |
|---|---|---|
| Volume | 볼륨 | Track or master fader level |
| Pan | 팬 | Stereo balance (`L` / `R`) |
| Gain | 게인 | Signal level adjustment in dB (`입력 게인`, `출력 게인`, `메이크업 게인`) |
| Mute | 음소거 | Silences a track or clip (`트랙 음소거`, `클립 음소거`) |
| Solo | 솔로 | Isolates selected track(s) for monitoring |
| Master | 마스터 | Main stereo output bus |
| Channel processing | 채널 프로세싱 | Per-track / master DSP strip (Input Gain, Parametric EQ, Compressor, Peak Limiter, Output Gain) |
| Parametric EQ | 파라메트릭 EQ | Multi-band equalizer |
| Compressor | 컴프레서 | Dynamic range compressor |
| Peak limiter | 피크 리미터 | Brickwall / peak ceiling stage |
| Threshold | 스레숄드 | Compressor/limiter activation level (`dB`) |
| Ratio | 비율 | Compression ratio |
| Makeup | 메이크업 게인 | Post-compression gain compensation (`dB`) |
| Effect Chain | 이펙트 체인 | Ordered sequence of channel effects and plugin inserts |
| Insert / Track inserts | 인서트 / 트랙 인서트 | Per-track or master plugin slots |
| Send | 센드 | Auxiliary send level (`Delay`, `Reverb`) |
| Wet | 믹스 | Dry/wet blend percentage on an insert slot |
| Bypass | 바이패스 | Passes audio through unmodified without removing the plugin or processor |
| Audio Plugins | 오디오 플러그인 | Plugin browser and management dialog |
| Instrument plugins | 악기 플러그인 | CLAP plugins that generate audio from MIDI notes |
| Effect plugins | 이펙트 플러그인 | CLAP plugins that process audio in an insert slot |
| Scan / Rescan | 검색 / 다시 검색 | Scans plugin search folders or installed sound packs |
| Open Editor / Parameters | 편집기 열기 / 파라미터 | Opens the clip editor or CLAP parameter table |

---

## 5. Audio Device, Export & Settings Terms

| English | 한국어 | Usage & Context |
|---|---|---|
| Sample Rate | 샘플레이트 | Audio clock rate (`44100 Hz`, `48000 Hz`, `96000 Hz`) |
| Bit Depth | 비트 깊이 | Export sample resolution (`16-bit PCM`, `24-bit PCM`, `32-bit float`) |
| Buffer Size / Period | 버퍼 크기 (프레임) | Audio device period in frames (`Requested period / frames`) |
| Latency | 지연 시간 | Processing or hardware delay (in samples or ms) |
| Output Range / Range | 출력 구간 | Export time span (`프로젝트 전체`, `반복 구간`, `선택한 클립 구간`) |
| Tail | 잔향 | Instrument release and reverb/delay decay after the end beat |
| Normalize | 노멀라이즈 | Scales peak level to `−1 dBFS` using explicit linear gain without compression |
| Dither | 디더 | TPDF dither applied when exporting integer PCM (`16-bit`, `24-bit`) |
| Destination | 저장 경로 | Output WAV file path |
| Cancel | 취소 | Cancels an in-progress export or dialog without overwriting existing files |

---

## 6. Sound Library, Synthesis & Sampler Terms

| English | 한국어 | Usage & Context |
|---|---|---|
| Instrument | 악기 | Playable sound source in the browser |
| Patch | 패치 | Specific articulation or curated sound configuration within an instrument entry |
| Preset | 프리셋 | Saved synth/sampler parameter state (`.json` / user preset) |
| SoundFont | SoundFont | `.sf2` / `.sf3` sample bank format (keep as `SoundFont`) |
| SFZ | SFZ | `.sfz` sample mapping format (keep as `SFZ`) |
| Synthesizer | 신시사이저 | Built-in procedural/subtractive/FM synth engine (`SYNTH` badge → `신스`) |
| Recorded | 샘플 | Recorded multi-sample acoustic/electric instrument (`RECORDED` badge → `샘플`) |
| Oscillator | 오실레이터 | Primary waveform generator (`사인파`, `톱니파`, `사각파`, `삼각파`) |
| Attack / Decay / Sustain / Release | 어택 / 디케이 / 서스테인 / 릴리스 | Amplitude or filter envelope stages |
| Low-pass | 로우패스 | Low-pass filter cutoff (`Hz`) |
| Detune | 디튠 | Fine pitch offset in cents |
| Portamento | 포르타멘토 | Pitch glide time between notes (`초`) |
| Monophonic / Legato | 모노포닉 / 레가토 | Single-voice playback and overlapping-note glide behavior |

---

## 7. Instrument Categories & Standard Instrument Names

### Categories

| English Category | 한국어 카테고리 |
|---|---|
| All instruments | 모든 악기 |
| Favorites | 즐겨찾기 |
| Recently used | 최근 사용 |
| Factory Instruments | 기본 악기 |
| Installed Sound Libraries | 설치된 사운드 라이브러리 |
| User Instruments | 사용자 악기 |
| Piano & Keys | 피아노 및 건반 |
| Strings | 현악기 |
| Woodwinds | 목관악기 |
| Brass | 금관악기 |
| Guitar & Bass | 기타 및 베이스 |
| Drums & Percussion | 드럼 및 타악기 |
| Synthesizers | 신시사이저 |
| World Instruments | 세계 민속악기 |
| Orchestral | 오케스트라 |
| Sound Effects & Textures | 효과음 및 텍스처 |

### Standard Instrument Names (Display Names in Guide & Documentation)

Internal instrument IDs (`factory.concert-grand`, `essential.upright-piano`, etc.), sample file paths, and original pack titles remain unchanged in project files and manifests. In Korean help, onboarding, and descriptive copy, use familiar Korean instrument names:

| English Name | 한국어 표기 |
|---|---|
| Concert Grand Piano / Grand Piano | 콘서트 그랜드 피아노 / 그랜드 피아노 |
| Upright Piano | 업라이트 피아노 |
| Electric Piano | 일렉트릭 피아노 |
| Pipe / Reed Organ | 오르간 / 리드 오르간 |
| Concert Harp | 콘서트 하프 |
| Acoustic Guitar (Nylon / Steel) | 어쿠스틱 기타 (나일론 / 스틸) |
| Electric Guitar | 일렉트릭 기타 |
| Electric Bass | 일렉트릭 베이스 |
| Upright Bass / Contrabass | 콘트라베이스 |
| Violin / Solo Violin | 바이올린 |
| Viola | 비올라 |
| Cello / Solo Cello | 첼로 |
| String Section | 현악 앙상블 / 스트링 섹션 |
| Concert Flute | 플루트 |
| Oboe | 오보에 |
| Clarinet | 클라리넷 |
| Bassoon | 바순 |
| French Horn | 호른 |
| Trumpet | 트럼펫 |
| Trombone | 트롬본 |
| Tuba | 튜바 |
| Marimba | 마림바 |
| Xylophone | 실로폰 |
| Vibraphone | 비브라폰 |
| Glockenspiel | 글로켄슈필 |
| Tubular Bells | 튜뷸러 벨 |
| Kalimba | 칼림바 |
| Balafon | 발라폰 |
| Studio Drum Kit / Drum Kit | 스튜디오 드럼 키트 / 드럼 키트 |
| Orchestral Percussion | 오케스트라 타악기 |

---

## 8. Non-Translatable Identifiers

Never translate or localize the following tokens:

- **Product & Organization Names**: `Reverie Ensemble`, `Ensemble`, `ensemble.`, `QuadraJ`, `Paper & Graphite`
- **Audio & Engine Technologies**: `CLAP`, `sfizz`, `FluidSynth`, `miniaudio`, `CoreAudio`, `CoreMIDI`, `Audio Units`, `VST3`, `SoundFont`, `SFZ`, `SMF`, `RIFF`, `PCM`, `TPDF`
- **Font & License Identifiers**: `IBM Plex Sans KR`, `IBM Plex Mono`, `Apache-2.0`, `LGPLv3`, `LGPL-2.1-or-later`, `BSD-2-Clause`, `MIT`, `MIT-0`, `CC0-1.0`, `CC-BY-4.0`, `GPL-3.0-or-later`, `SIL OFL`
- **Units, Ratios & Music Symbols**: `BPM`, `Hz`, `kHz`, `dB`, `dBFS`, `ms`, `MiB`, `GiB`, `Q`, `LFO`, `FM`, `1/4`, `1/8`, `1/16`, `1/32`, `3/4`, `4/4`, `6/8`, `7/8`, `C4`, `A4`, `MIDI 60`
- **Internal IDs & User Data**: Instrument IDs (`factory.*`, `essential.*`), Plugin IDs (`org.surge-synth-team.surge-xt`), user track names, clip names, and file paths.
