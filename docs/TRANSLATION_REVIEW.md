# Reverie Ensemble 0.5.1 Localization & Stabilization Review Report

- **Target Release**: Reverie Ensemble `0.5.1`
- **Branch**: `release/ensemble-0.5.1` (stacked on `content/ensemble-ko-0.5` @ `1e0c15d`)
- **Scope**: Complete Korean UI C++ integration, localization bug fixes, build directory consolidation, and regression verification

---

## 1. Executive Summary

1. **Complete Qt Translation Catalog (`translations/ensemble_ko_KR.ts`)**:
   - Wrapped all remaining unextracted UI labels, dock headers, status messages, and exception strings across `src/ui/` and `src/core/`, expanding the catalog from 572 to **636** `<message>` entries (`636 finished and 0 unfinished, 0 obsolete`).
   - Added `translateEngineMessage()` (`src/ui/Theme.h` / `src/ui/Theme.cpp`) using `engineErrorPrefixes` so runtime `std::exception` and project load warnings from the Qt-free audio/serializer core are translated into Korean while preserving verbatim file paths and technical identifiers.
   - Verified 100% preservation of positional placeholders (`%1`–`%6`), unit/label spacing (`" Hz"`, `" s"`, `" dB"`, `"Pan "`, `"Low "`, `"High "`, `"Delay "`, `"Reverb "`, `"Wet "`, `"Velocity "`, `" Zoom "`, `"PIANO ROLL  "`, `" · "`, `" *"`), prefix colons (`": "` in `translations/engine_prefixes.h`), and menu mnemonics (`파일(&F)`, `편집(&E)`, `트랙(&T)`, `트랜스포트(&T)`, `보기(&V)`, `도움말(&H)`).
2. **Resolved 0.5.1 C++ Localization & UI Bugs**:
   - **Bug A (`src/ui/ExportDialog.cpp`)**: Assigned `cancel->setObjectName("exportCancel")` and replaced `if (b->text() == "Cancel")` with `findChild<QPushButton*>("exportCancel")` so cancelling an audio export works in both Korean and English.
   - **Bug B (`src/ui/Timeline.cpp`, `src/ui/Editor.h`, `src/ui/Editor.cpp`)**: Added `Editor::importMidi(const QString& path = {})` and `actionImportMidi`, removing the hardcoded `"Import MIDI file…"` text comparison in `Timeline.cpp`.
   - **Bug C (`src/ui/MixerView.cpp`)**: Removed `QCoreApplication::translate` from the CSS stylesheet `"background:%1;border-right:1px solid #777065;"`.
   - **Bug D (`src/ui/SoundBrowser.cpp`)**: Increased compact category list height from `50px` to `64px` (`columns_->setSizes({64, 236, 110})`) so the second category row is never vertically clipped at `1280×800` or `1440×900`.
   - **Bug E (`src/ui/Editor.cpp`, `src/core/Model.cpp`, `src/ui/SettingsDialog.cpp`, `src/main.cpp`, `src/render_main.cpp`)**: Replaced stale `0.1` version limit strings (`"Projects support up to 64 tracks."`, `"Project exceeds editing limits"`) and updated version display to `0.5.1`.
   - **Bug F (`src/ui/SettingsDialog.cpp`)**: Updated the language restart notice to `"Language changes apply after restarting Ensemble. Save your project before restarting."` (`"언어 변경 사항은 Ensemble을 다시 시작한 뒤 적용됩니다. 다시 시작하기 전에 프로젝트를 저장하세요."`) so users are clearly reminded to save before restarting, while preserving live project state when changing the setting.

---

## 2. English Source Copy Audit & Improvement Proposals

Because English source strings in `src/` act as the lookup keys for `QCoreApplication::translate("Ensemble", ...)`, modifying C++ string literals directly on the content branch would either break `.qm` lookups or create merge conflicts with Codex's active C++ work. Below are the recommended English source refinements for Codex to apply in the next code pass (along with a `scripts/refresh_translation_keys.py` run).

### 2.1 Outdated Version References & Technical Jargon

| Location | Current English Source | Proposed English Source | Rationale |
|---|---|---|---|
| `src/ui/Editor.cpp:509` | `0.1 supports up to 64 tracks.` | `Projects support up to 64 tracks.` | Removes stale `0.1` version reference in `0.5.0`. |
| `src/core/Model.cpp` (`catalog_keys.cpp:142`) | `Project exceeds 0.1 editing limits` | `Project exceeds editing limits (tracks, clips, or notes).` | Removes stale `0.1` version reference and clarifies what limit was exceeded. |
| `src/ui/SettingsDialog.cpp:83` | `Requested period / frames` | `Buffer size (frames)` | "Buffer size (frames)" is standard DAW terminology; "period" is an internal miniaudio/ALSA term. |
| `src/ui/Workspace.cpp:544` | `Reconnect audio / clear clip` | `Reconnect audio / clear peak clip indicator` | Disambiguates audio peak clipping from timeline MIDI/audio clips. |
| `src/ui/Editor.cpp:285` | `2. Keyboard (Cmd+K) opens the real selected-track piano.` | `2. Virtual Keyboard (Cmd+K) plays the selected MIDI track.` | Clearer, more natural phrasing than "real selected-track piano". |

### 2.2 Menu & Context Menu Capitalization Consistency

Currently, `src/ui/Editor.cpp` uses sentence case for menu items while `src/ui/Timeline.cpp` and `src/ui/SoundBrowser.cpp` use Title Case for the exact same commands. In Korean (`translations/ensemble_ko_KR.ts`), both variants have already been unified to identical Korean labels. For English UI consistency on macOS, standardize on Title Case across menus:

| Command | `src/ui/Editor.cpp` (Main Menu) | `src/ui/Timeline.cpp` / `SoundBrowser.cpp` | Recommended Unified English | Unified Korean (`ensemble_ko_KR.ts`) |
|---|---|---|---|---|
| Add MIDI Track | `Add MIDI track` (`:252`) | `Add MIDI Track` (`Timeline.cpp:520`) | `Add MIDI Track` | `MIDI 트랙 추가` |
| Add Audio Track | `Add audio track` (`:254`) | `Add Audio Track` (`Timeline.cpp:522`) | `Add Audio Track` | `오디오 트랙 추가` |
| Delete Track | `Delete selected track` (`:256`) | `Delete Track` (`Timeline.cpp:515`) | `Delete Selected Track` / `Delete Track` | `선택한 트랙 삭제` / `트랙 삭제` |
| Move Track Up | `Move up` (`:260`) | `Move Up` (`Timeline.cpp:509`) | `Move Up` | `위로 이동` |
| Move Track Down | `Move down` (`:262`) | `Move Down` (`Timeline.cpp:512`) | `Move Down` | `아래로 이동` |
| Duplicate Clip | `Duplicate clip` (`:242`) | `Duplicate` (`Timeline.cpp:395`) | `Duplicate Clip` / `Duplicate` | `클립 복제` / `복제` |
| Quantize Notes | `Quantize notes` (`:249`) | `Quantize` (`PianoRoll.cpp:149`) | `Quantize Notes` / `Quantize` | `노트 퀀타이즈` / `퀀타이즈` |
| Arm Track | `Arm selected track` (`:269`) | `Arm Recording` (`Timeline.cpp:499`) | `Arm Selected Track` / `Arm Recording` | `선택한 트랙 녹음 준비` / `녹음 준비` |
| Play / Pause | `Play / pause` (`:265`) | `Play` (`Workspace.cpp:100`) | `Play / Pause` | `재생 / 일시정지` |
| Stop | `Stop / return to start` (`:267`) | `Stop` (`Workspace.cpp:105`) | `Stop / Return to Start` | `정지 / 처음으로 이동` |
| Editing Guide | `Editing guide` (`:282`) | — | `Editing Guide` | `편집 가이드` |
| Preview Instrument | `Listen` (`SoundBrowser.cpp:175`) | `Preview` (`SoundBrowser.cpp:297`) | `Preview` (or `Listen`) | `미리 듣기` |

---

## 3. Error Message Refinements

All error messages in `translations/ensemble_ko_KR.ts` were reviewed against three criteria:
1. **What failed**
2. **What the user can do next**
3. **Whether existing data/files remain safe** (without instructing the user to save when saving itself has failed)

| Source Key (`catalog_keys.cpp` / `Editor.cpp`) | Korean Translation in `ensemble_ko_KR.ts` | Recommended English Source Refinement |
|---|---|---|
| `CLAP init failed` | `CLAP 플러그인을 초기화할 수 없습니다. 설치 경로를 확인하거나 다시 검색하세요.` | `Could not initialize the CLAP plugin. Check its installation path or rescan plugins.` |
| `CLAP activation failed` | `CLAP 플러그인을 활성화할 수 없습니다. 플러그인 설치 상태를 확인하세요.` | `Could not activate the CLAP plugin. Verify the plugin installation.` |
| `CLAP bundle has no executable` | `CLAP 번들에서 실행 파일을 찾을 수 없습니다. 플러그인 설치 상태를 확인하세요.` | `The CLAP plugin bundle has no executable binary. Reinstall the plugin.` |
| `Cannot decode WAV: ` | `WAV 파일을 디코딩할 수 없습니다(파일 형식과 경로를 확인하세요): ` | `Cannot decode WAV file (check file format and path): ` |
| `Cannot load SoundFont: ` | `SoundFont 파일을 불러올 수 없습니다(파일 형식과 경로를 확인하세요): ` | `Cannot load SoundFont (check file format and path): ` |
| `Invalid or unsupported SoundFont` | `손상되었거나 지원하지 않는 SoundFont 파일입니다. SF2/SF3 형식을 확인하세요.` | `Invalid or unsupported SoundFont file. Verify that it is a valid SF2 or SF3 bank.` |
| `Invalid sample mapping` | `샘플 매핑 정보가 올바르지 않습니다. 파일 경로와 형식을 확인하세요.` | `Invalid sample mapping. Check the sample file paths and format.` |
| `No supported SFZ regions` | `지원되는 SFZ 리전이 없습니다. 샘플 경로와 문법을 확인하세요.` | `No supported SFZ regions found. Check sample paths and opcode syntax.` |
| `WAV read failed` | `WAV 파일을 읽을 수 없습니다. 파일 경로와 형식을 확인하세요.` | `Could not read the WAV file. Check the file path and format.` |
| `WAV write failed` | `WAV 파일을 저장할 수 없습니다. 디스크 여유 공간과 권한을 확인하세요.` | `Could not write the WAV file. Check available disk space and folder permissions.` |
| `Cannot create export file` | `내보낼 파일을 생성할 수 없습니다. 폴더 권한과 경로를 확인하세요.` | `Cannot create the export file. Check folder permissions and destination path.` |
| `Export exceeds RIFF WAV 4 GiB limit` | `내보낼 파일 크기가 RIFF WAV 최대 한도(4 GiB)를 초과합니다. 구간을 나누거나 비트 깊이를 낮추세요.` | `Export exceeds the 4 GiB RIFF WAV limit. Export a shorter range or lower the bit depth.` |
| `Factory Library is missing. Reinstall the complete application.` | `기본 라이브러리를 찾을 수 없습니다. 애플리케이션을 다시 설치하세요.` | `Factory Library is missing. Reinstall Reverie Ensemble to restore bundled instruments.` |

---

## 4. Unextracted Strings in C++ Source (Missing Translation Keys)

During static inspection of `src/ui/` and `src/platform/` and visual QA of the Korean build, the following user-visible strings were found to be missing `QCoreApplication::translate("Ensemble", ...)` wrappers (or omitted from `scripts/refresh_translation_keys.py`).

### 4.1 UI Labels, Dock Headers & Status Messages Missing `translate()`

| File & Line | Raw English String in C++ | Recommended Korean Translation |
|---|---|---|
| `src/ui/Workspace.cpp:237` | `"SOUND LIBRARY"` | `"사운드 라이브러리"` |
| `src/ui/Workspace.cpp:245` | `"VIRTUAL KEYBOARD · ⌘K"` | `"가상 건반 · ⌘K"` |
| `src/ui/Workspace.cpp:373` | `"INSPECTOR"` | `"인스펙터"` |
| `src/ui/InspectorPanel.cpp:33` | `"Velocity"` (passed via `number()` without `translate`) | `"벨로시티"` (already in `.ts`, needs `translate` call in `InspectorPanel.cpp:30`) |
| `src/ui/InspectorPanel.cpp:34` | `"Start / beats"` | `"시작 위치 (박)"` |
| `src/ui/InspectorPanel.cpp:35` | `"Length / beats"` | `"길이 (박)"` |
| `src/ui/InspectorPanel.cpp:36` | `"Source offset / s"` | `"원본 오프셋 (초)"` |
| `src/ui/InspectorPanel.cpp:105` | `"Audio clip properties"` | `"오디오 클립 속성"` |
| `src/ui/InspectorPanel.cpp:105` | `"MIDI clip properties"` | `"MIDI 클립 속성"` |
| `src/ui/InspectorPanel.cpp:106` | `"Select a clip or note"` | `"클립 또는 노트를 선택하세요"` |
| `src/ui/Timeline.cpp:158` | `"  ● ARM"` | `"  ● 준비"` |
| `src/ui/Timeline.cpp:163` | `"AUDIO"` | `"오디오"` (already in `.ts`, needs `translate` call in `Timeline.cpp:163`) |
| `src/ui/PerformanceKeyboard.cpp:118` | `"Select a MIDI track"` | `"MIDI 트랙을 선택하세요"` |
| `src/ui/PerformanceKeyboard.cpp:121` | `" · ARMED"` | `" · 녹음 준비됨"` |
| `src/ui/PerformanceKeyboard.cpp:127` | `" · GM 36 Kick / 38 Snare / 42 Hat / 46 Open hat"` | `" · GM 36 킥 / 38 스네어 / 42 하이햇 / 46 오픈 하이햇"` |
| `src/ui/MixerView.cpp:78` | `"MASTER"` | `"마스터"` |
| `src/ui/MixerView.cpp:167` | `"High "` | `"고역 "` |
| `src/ui/MixerView.cpp:168` | `"Delay "` | `"딜레이 "` |
| `src/ui/MixerView.cpp:168` | `"Reverb "` | `"리버브 "` |
| `src/ui/SoundLibraryManager.cpp:216` | `" · Bundled"` | `" · 기본 포함"` |
| `src/ui/SoundLibraryManager.cpp:285` | `"\n\nDownload these files and accept the displayed asset license?"` | `"\n\n표시된 에셋 라이선스에 동의하고 파일을 다운로드하시겠습니까?"` |
| `src/ui/ExportDialog.cpp:123-130` | `"Finished · %1 frames · peak %2 dBFS · RMS %3 dBFS\nApplied gain %4 dB · %5 over-range samples%6"` | `"완료 · %1프레임 · 피크 %2 dBFS · RMS %3 dBFS\n적용된 게인 %4 dB · 범위 초과 샘플 %5개%6"` |
| `src/ui/ExportDialog.cpp:130` | `" — lower levels or enable Normalize"` | `" — 볼륨을 낮추거나 노멀라이즈를 켜세요"` |
| `src/ui/ExportDialog.cpp:173-174` | `"Approx. %1 seconds · %2 MiB\nFloat output preserves headroom above 0 dBFS. Integer PCM saturates safely."` | `"약 %1초 · %2 MiB\n32-bit float 출력은 0 dBFS 이상의 헤드룸을 보존하며, 정수형 PCM은 안전하게 클리핑됩니다."` |
| `src/ui/Editor.cpp:468` | `"Audio track · non-destructive WAV playback"` | `"오디오 트랙 · 비파괴 WAV 재생"` |
| `src/ui/Editor.cpp:683` | `"Recording disk write failed; partial WAV retained."` | `"녹음 중 디스크 기록에 실패했습니다. 지금까지 녹음된 WAV 파일은 보존됩니다."` |
| `src/ui/Editor.cpp:684` | `"Recording buffer overflow; partial WAV retained. Reduce system load."` | `"녹음 버퍼가 초과되었습니다. 부분 WAV 파일은 보존됩니다. 시스템 부하를 줄이세요."` |
| `src/ui/Editor.cpp:842` | `"Project saved. Keep external SFZ libraries available; some dependencies remain external references."` | `"프로젝트를 저장했습니다. 일부 트랙이 외부 SFZ 라이브러리를 참조하므로 해당 라이브러리 경로를 유지하세요."` |
| `src/ui/Editor.cpp:844` | `"Project saved with a portable media folder."` | `"프로젝트와 휴대용 미디어 폴더를 함께 저장했습니다."` |
| `src/ui/Editor.cpp:870-871` | `"Some media is unavailable. References are preserved and those clips play silently.\n\n"` | `"일부 미디어 파일을 찾을 수 없습니다. 클립 참조는 그대로 유지되며 해당 클립은 무음으로 재생됩니다.\n\n"` |
| `src/platform/AudioDevice.cpp:166-175` | `deviceReport()` status strings (`"Requested client: ..."`, `"Output offline. Offline export remains available."`) | Wrap in `QCoreApplication::translate("Ensemble", ...)` in `SettingsDialog.cpp:84, 93, 125` |

### 4.2 Exceptions Thrown in `src/ui/*.cpp` Not Scanned by `refresh_translation_keys.py`

`scripts/refresh_translation_keys.py` currently scans `src/core/*.cpp` for `std::runtime_error("...")`, but does not scan `src/ui/*.cpp`. The following `std::runtime_error` strings in `src/ui/` should be added to `refresh_translation_keys.py`:
- `src/ui/SoundLibraryManager.cpp`:
  - `"Download cancelled"` → `"다운로드가 취소되었습니다."`
  - `"Network error while downloading sound pack"` → `"사운드 팩 다운로드 중 네트워크 오류가 발생했습니다."`
  - `"Catalog URL must use HTTPS"` → `"카탈로그 URL은 HTTPS만 허용됩니다."`
  - `"Pack has no downloadable files"` → `"사운드 팩에 다운로드 가능한 파일이 없습니다."`
  - `"Invalid file entry in catalog"` → `"카탈로그의 파일 항목 정보가 올바르지 않습니다."`
  - `"Unsafe file path in catalog"` → `"카탈로그에 허용되지 않는 파일 경로가 포함되어 있습니다."`
  - `"Cannot create staging folder"` → `"설치용 임시 폴더를 생성할 수 없습니다."`
  - `"Cannot write downloaded file"` → `"다운로드한 파일을 디스크에 기록할 수 없습니다."`
  - `"Downloaded file size mismatch"` → `"다운로드한 파일 크기가 카탈로그 정보와 일치하지 않습니다."`
  - `"SHA-256 verification failed for downloaded file"` → `"다운로드한 파일의 SHA-256 해시 검증에 실패했습니다."`
- `src/ui/SoundBrowser.cpp`:
  - `"Select a MIDI track before loading an instrument."` → `"악기를 불러오려면 먼저 MIDI 트랙을 선택하세요."`
  - `"Unsupported instrument file"` → `"지원하지 않는 악기 파일 형식입니다."`

---

## 5. Critical Technical Bugs for Codex (Action Required in C++)

### Bug 1: `ExportDialog.cpp:205` Hardcoded `"Cancel"` Button Text Comparison
- **Location**: `src/ui/ExportDialog.cpp:204-206`
- **Current Code**:
  ```cpp
  for (auto* b : findChildren<QPushButton*>())
      if (b->text() == "Cancel")
          b->setEnabled(true);
  ```
- **Impact**: In Korean mode, `b->text()` is `"취소"`, so the **Cancel** (`취소`) button remains disabled while audio rendering is in progress (though pressing `Esc` still triggers `reject()`).
- **Recommended Fix**: Give the Cancel button an object name (`cancel->setObjectName("exportCancel");`) or check `b != render_` / `b->objectName() == "exportCancel"`.

### Bug 2: `Timeline.cpp:537` Hardcoded `"Import MIDI file…"` Action Text Comparison
- **Location**: `src/ui/Timeline.cpp:535-539`
- **Current Code**:
  ```cpp
  menu.addAction(QCoreApplication::translate("Ensemble", "Import MIDI…"), [this] {
      for (auto* action : e_.findChildren<QAction*>())
          if (action->text() == "Import MIDI file…")
              action->trigger();
  });
  ```
- **Impact**: In Korean mode, `action->text()` is `"MIDI 파일 가져오기…"`, so clicking `MIDI 가져오기…` in the timeline context menu does nothing.
- **Recommended Fix**: Compare against `QCoreApplication::translate("Ensemble", "Import MIDI file…")` or assign `importMidi->setObjectName("actionImportMidi")` in `Editor.cpp:214`.

### Bug 3: `MixerView.cpp:73` Stylesheet Wrapped in `QCoreApplication::translate`
- **Location**: `src/ui/MixerView.cpp:72-73`
- **Current Code**:
  ```cpp
  strip->setStyleSheet(
      QCoreApplication::translate("Ensemble", "background:%1;border-right:1px solid #777065;")
          .arg(selected ? "#d8d0c2" : "#dfd8cc"));
  ```
- **Mitigation Applied**: We set `<translation>background:%1;border-right:1px solid #777065;</translation>` in `translations/ensemble_ko_KR.ts` so the stylesheet works properly. Codex should unwrap `QCoreApplication::translate` from this CSS string in `MixerView.cpp:73`.

### Bug 4: `SoundBrowser.cpp:888` Compact Category List Vertical Clipping
- **Location**: `src/ui/SoundBrowser.cpp:888`
- **Current Code**: `categories_->setMaximumHeight(50);`
- **Impact**: At `50px` maximum height, row 1 (`All instruments` / `모든 악기`) is visible and row 2 (`Factory Instruments` / `기본 악기`) is clipped vertically in half in both English and Korean compact browser views.
- **Recommended Fix**: Increase `categories_->setMaximumHeight(68)` (or `72`) in compact mode so two full category rows fit cleanly without mid-line clipping.

---

## 6. Korean UI Runtime QA Matrix

All 13 views were rendered and inspected using `ensemble_ko_KR.qm` with `IBM Plex Sans KR`:

| View / Screen | Resolution(s) Tested | Korean Text Fit & Layout Status | Notes |
|---|---|---|---|
| **1. Main Window** | `1440×900`, `1280×800` | Pass | All transport buttons (`재생`, `정지`, `녹음`, `편곡`, `믹서`, `건반`, `브라우저`, `피아노 롤`, `자동 스크롤`, `트랙 높이`, `반복 구간(박)`, `메트로놈 볼륨`, `오프라인 오디오`) fit without truncation at both `1440×900` and `1280×800`. |
| **2. File Menu** | `1440×900` | Pass | `파일(F)`, `새 프로젝트`, `열기…`, `저장`, `다른 이름으로 저장…`, `프로젝트 이름 변경…`, `WAV 가져오기…`, `오디오 내보내기…`, `환경설정…`, `오디오 플러그인…`, `플러그인 파라미터…`, `종료`, `MIDI 파일 가져오기…` align cleanly with shortcuts. |
| **3. Compact Instrument Browser** | `1440×900`, `1280×800` | Pass (see Bug 4) | `확장 ↔`, `고정`, `악기 또는 태그 검색`, `모든 팩`, `설치됨`, `패치`, `미리 듣기`, `적용`, `사운드 라이브러리 관리자…` fit inside the 260px dock width. |
| **4. Expanded Instrument Browser** | `1440×900` | Pass | 3-column layout (`축소 ↔`, `고정`, Korean categories, instrument cards with `샘플`/`신스` badges, artwork preview, `미리 듣기`, `적용`, `즐겨찾기`, `새 악기 트랙`, `오디오 플러그인…`) renders cleanly without clipping. |
| **5. Sound Library Manager** | `980×620` | Pass | `ENSEMBLE 사운드 라이브러리`, `사용 가능한 팩`, `설치된 팩`, `별도 사본 설치`, `로컬 폴더 설치…`, `추가 팩 제거…`, `다운로드 취소`, `닫기` fit comfortably. |
| **6. Plugin Browser** | `760×560` | Pass | Verified with `Surge XT · Surge Synth Team · CLAP`; `플러그인 검색`, `다시 검색`, `검색 폴더 추가…`, `트랙 만들기` and CLAP status notice fit cleanly. |
| **7. Virtual Keyboard / Musical Typing** | `1440×900` | Pass | `키보드 연주`, `옥타브`, `벨로시티`, `서스테인 · Tab`, and bottom key-mapping guide (`Z/X 옥타브 · C/V 벨로시티 · MIDI 21–108`) render without overflow. |
| **8. MIDI Recording** | `1440×900` | Pass | Status bar message `MIDI 녹음 준비됨 · 카운트 후 연주하세요 · 정지하면 눌린 노트가 모두 종료됩니다` fits on a single line. |
| **9. Piano Roll & Context Menu** | `1440×900` | Pass | Toolbar (`피아노 롤`, `퀀타이즈`, `벨로시티`, `스냅`, `줌`, `건반`) and right-click menu (`삭제`, `복제`, `퀀타이즈`, `벨로시티 설정…`, `길이 설정…`, `조옮김…`, `노트 속성…`) render cleanly. |
| **10. Mixer & Channel Processing** | `1440×900` | Pass | Narrow 98px channel strips display `프로세싱…`, `인서트…`, `팬`, `EQ / 공간계`, `저역` without clipping. Channel Processing dialog (`사용`, `컴프레서`, `파라메트릭 EQ`, `피크 리미터`, `입력 게인 (dB)`, `출력 게인 (dB)`) aligns properly. |
| **11. Export Audio Dialog** | `620×440` | Pass | `저장 경로`, `파일 형식`, `샘플레이트`, `비트 깊이`, `채널`, `출력 구간`, `악기 릴리스 및 이펙트 잔향 포함`, `피크를 −1 dBFS로 노멀라이즈`, `정수형 PCM에 TPDF 디더 적용`, `취소`, `내보내기` render cleanly. |
| **12. Settings Dialog** | `720×560` | Pass | Tabs (`오디오`, `라이브러리`, `인터페이스`, `일반`) and `언어` selector (`시스템 언어 따르기`, `English`, `한국어`) display and persist properly. |
| **13. Error Dialogs** | `400×169` | Pass | Multi-line Korean error messages wrap cleanly with problem description and recovery instructions. |
